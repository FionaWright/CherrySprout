# Path Guiding

Data structure split into two parts

SD-Tree:  
    Non-overlapping Binary Tree (Spatio) over the scene which has a QuadTree (Directional) at each leaf node

PPG involves a training phase of M iterations after which the data structure becomes read-only to avoid overhead  
M commonly about 128-512 passes 

Binary tree alternates between splitting x,y,z.  
At the end of each iteration the SD-Tree is replaced using the previous one as a guide, all spatial nodes should encounter roughly equal number of path vertices during rendering. And all directional nodes should contain roughly equal flux  
Maybe implement this adaptive tree algorithm later  

QuadTree maps discrete $(cos(\theta),\phi)$ to incident radiance  
QuadTree represents world-space aligned cylindrical coordinates 

Debugging Tip:
    Incident radiance can be stored as float3 if wanted, even though it only needs the luminance (?)
    This means you can render a 3D sphere probe at each leaf node position which should show the surroundings 

When a path is completed (from NEE or indirect), the radiance arriving at each of its vertices is recorded in the SD-Tree. Can re-use GDPT PathVertex here  
Information needed in each vertex:
- Position
- Direction
- Outgoing Radiance (Lo)

Given outgoing radiance $L_o(x_{i+1}, \omega_i)$ where $\omega_i$ is the direction $x_{i} \to x_{i+1}$:  
$L_i(x_i, \omega_i) = f(x_i, \omega_i, \omega_o) L_o(x_{i+1}, \omega_i) | n \cdot \omega_i | p^{-1}(\omega_i)$  
Double check all of this, probably wrong

```cpp
float3 SampleQuadTree(Node node)
{
    if (node.IsLeaf)
    {
        float2 uv = RandUniformPositionIn(rngInfo, node); // ?
        return CylinderToSphere(uv);
    }

    Node child = SampleChildByEnergy(node); // ?
    return SampleQuadTree(child);
}

float QuadTreePDF(Node node, float3 wi)
{
    if (node.IsLeaf)
        return 1 / (4 * PI);

    float2 uv = SphereToCylinder(wi);
    Node child = GetChild(node, uv);

    float beta = 4 * GetFlux(child) / GetFlux(node);
    return beta * QuadTreePDF(child, wi);
}
```

Do not train the SD-Tree using NEE

Since early iterations are naive and poorly guided, they can lead to worse noise than if they were discarded. Thus rather than allocating an equal number of samples to each iteration, PPG doubles the number of samples each iteration  
Thus for the M training iterations, $2^M - 1$ samples are taken
The algorithm then discards all images except the one produced by the final iteration. This is of course wasteful so a better system is:  

## Extensions

### Inverse Variance Weighted Sample Combination

As images $I^1 \dots I^M$ are independent RV, the optimal per-pixel combination weights are proportional to the inverse pixel variance

For iteration $k$ at pixel $p$   

$w^k(p) = \mathbb{V}(I^k(p))^{-1}$  

Unfortunately you cannot know the variance of each individual pixel, trying to use an estimate it will only introduce correlation/bias

Instead combine the images weighted by their mean pixel variance 

$\bar w^k = \sum w^k(p)$  

$I = (\sum \bar w^k)^{-1} \sum \bar w^k I^k$ 

As rendering progresses, the variance estimate approaches the true variance of the pixel value

They recommend only combining the last 4 images. This means you only discard ~6.25% of the samples

### Spatio-Directional Splatting/Filtering

You can image this data structure will introduce artifacts along the edges of the nodes

Instead of only recording a position $x$ and direction $w_i$, you instead record into nodes that fall in a spatial/directional neighbourhood

When traversing the spatial tree, keep track of the spatial footprint and corresponding volume $V$ of the leaf containing $x$  
This footprint determines the filter radius  
Then traverse again, this time visiting each node that has non-zero volume overlap $V_o$ with the spatial footprint from before. Record the radiance estimate weighted by the fraction of overlapped volume $V_o / V$  
Directional filtering works similarly except as area filtering over the cylindrical domain  

The filtering operation can be very expensive however, this cost can be avoided at the expense of slightly worse quality by replacing the deterministic filtering that traverses entire sub-trees with stochastic filtering that traverses only towards a single leaf  
After obtaining the footprint, the position is jittered uniformly within the footprints volume and then recorded in the tree without weighting the radiance by $V_o / V$ 

Using stochastic filtering for only one of the spatio/directional trees results in the most speedup. Recommended to do stochastic spatial and deterministic directional 

### MIS Optimization

Simple model is to sample PPG according to $\alpha$   
$\alpha=0.5$ means 50/50 sampling

However it would be better to have $\alpha$ vary spatially to account for the fact BxDF sampling is more suitable in some regions

Replace $\alpha$ with a learned function $\alpha(x)$ 

$\hat alpha = arg \ min_{\alpha} D(p || \hat q ; \alpha)$ 

For D, use the Kullback-Leibler divergence 

The KL divergence between the ideal PDF $p$ and the learned PDF $\hat q$ is:  
$D_{KL}(p || \hat q ; \alpha) = \int_S p(\omega_i) log \dfrac{p(w_i)}{\hat q(\omega_i;\alpha)} \ d \omega_i$ 

Minimize the KL divergence by setting its gradient to 0. This is a classic machine learning problem (See stochastic gradient descent)  
Use "Adam" to optimize $\alpha$  

Will ignore rest of implementation for now, likely not needed. There's source code for the algorithm if wanted

## Data Structure

Binary Tree starts with an AABB around the scene

Then each iteration splits x,y,z,x,y,z,...

Tree terminates when at max depth ?

All that needs to be known at each node is what fraction of the box is where it splits

For a tree at max depth $N$, requires $2^N - 1$ nodes
Children of node at index $k$ are at indices $2k+1, 2k+2$ 

For QuadTree, children are at $4k+1, 4k+2, 4k+3, 4k+4$  

```cpp
#define SD_TREE_MAX_DEPTH 16
#define QUAD_TREE_MAX_DEPTH 8
#define SD_TREE_MAX_NODES (pow(2, SD_TREE_MAX_DEPTH) - 1)
#define QUAD_TREE_MAX_NODES ((pow(4, QUAD_TREE_MAX_DEPTH) - 1) / 3)

struct SdTreeNode
{
    float SplitFraction;
    int QuadTreeLocalIdx;
};

struct QuadTreeNode // Could be templated to allow for different internal values
{
    uint IsLeaf;
    float Luminance; 
    uint SampleCount;
};

struct CbvSdTreeInfo
{
    float3 SceneAabbMin;
    float3 SceneAabbMax;
};

StructuredBuffer<SdTreeNode> gSdTree;
StructuredBuffer<QuadTreeNode> gQuadTrees;
ConstantBuffer<CbvSdTreeInfo> gCbvSdTree;

uint GetQuadTreeRootGlobalIndex(uint rootLocalIdx)
{
    return rootLocalIdx * QUAD_TREE_MAX_NODES;
}

void InitializeSdTree()
{
    gSdTree[i].QuadTreeLocalIdx = 0;

    gQuadTrees[0].IsLeaf = true;
    gQuadTrees[0].Luminance = 0.0f;
    gQuadTrees[0].SampleCount = 1;
}

bool IsOnSdTreeNodeLeftSide(float3 mid, float3 pos, uint iterationIdx)
{
    if (iterationIdx % 3 == 0)
        return pos.x < mid.x;

    if (iterationIdx % 3 == 1)
        return pos.y < mid.y;

    return pos.z < mid.z;
}

int FindQuadTreeIdx(float3 pos)
{
    if (any(pos < gCbvSdTree.SceneAabbMin) || any(pos > gCbvSdTree.SceneAabbMax))
        return -1;

    float3 min = gCbvSdTree.SceneAabbMin;
    float3 max = gCbvSdTree.SceneAabbMax;

    uint i = 0;
    while (i < SD_TREE_MAX_NODES)
    {
        if (gSdTree[i].QuadTreeLocalIdx != -1)
            return gSdTree[i].QuadTreeLocalIdx;

        float3 mid = lerp(min, max, gSdTree[i].SplitFraction);
        if (IsOnSdTreeNodeLeftSide(mid, pos, i))
        {
            max = mid;
            i = 2 * i + 1;
        }
        else
        {
            min = mid;
            i = 2 * i + 2;
        }
    }

    return -1;
}

#define COS_THETA_MIN -1.0f
#define COS_THETA_MAX 1.0f
#define PHI_MIN 0.0f
#define PHI_MAX (2.0f * PI)

float3 CylinderToSphere(float cosTheta, float phi)
{

}

float2 SphereToCylinder(float3 dir)
{

}

float3 SampleQuadTree(RngInfo rngInfo, uint quadTreeRootGlobalIdx, out float pdf)
{
    float2 min = float2(COS_THETA_MIN, PHI_MIN);
    float2 max = float2(COS_THETA_MAX, PHI_MAX);

    pdf = 1.0f;

    uint localIdx = 0;
    while (localIdx < QUAD_TREE_MAX_NODES)
    {
        uint globalIdx = quadTreeRootGlobalIdx + localIdx;

        if (gQuadTrees[globalIdx].IsLeaf)
        {
            float2 boundsRange = max - min;
            float2 uv = float2(Rand01(rngInfo), Rand01(rngInfo));
            uv = uv * boundsRange + min;

            pdf *= 1 / (4.0f * PI); 

            return CylinderToSphere(uv.x, uv.y);
        }

        // Assuming (0,0) is SW and (1,1) is NE

        uint localIdx_NW = 4 * localIdx + 1;
        uint localIdx_NE = 4 * localIdx + 2;
        uint localIdx_SE = 4 * localIdx + 3;
        uint localIdx_SW = 4 * localIdx + 4;

        uint globalIdx_NW = quadTreeRootGlobalIdx + localIdx_NW;
        uint globalIdx_NE = quadTreeRootGlobalIdx + localIdx_NE;
        uint globalIdx_SE = quadTreeRootGlobalIdx + localIdx_SE;
        uint globalIdx_SW = quadTreeRootGlobalIdx + localIdx_SW;

        float Li_NW = gQuadTrees[globalIdx_NW].Luminance;
        float Li_NE = gQuadTrees[globalIdx_NE].Luminance;
        float Li_SE = gQuadTrees[globalIdx_SE].Luminance;
        float Li_SW = gQuadTrees[globalIdx_SW].Luminance;
        float Li_Total = Li_NW + Li_NE + Li_SE + Li_SW;

        float2 mid = lerp(min, max, 0.5f);
        
        uint selectedLocalIdx;
        float r01 = Rand01(rngInfo);
        if (r01 < Li_NW / Li_Total)
        {
            selectedLocalIdx = localIdx_NW;
            min.y = mid.y; // Y-Up
            max.x = mid.x; // X-Down
        }
        else if (r01 < Li_NE / Li_Total)
        {
            selectedLocalIdx = localIdx_NE;
            min = mid; 
        }
        else if (r01 < Li_SE / Li_Total)
        {
            selectedLocalIdx = localIdx_SE;
            min.x = mid.x; // X-Up
            max.y = mid.y; // Y-Down
        }
        else // SW
        {
            selectedLocalIdx = localIdx_SW;
            max = mid; 
        }

        uint selectedGlobalIdx = quadTreeRootGlobalIdx + selectedLocalIdx;

        pdf *= 4 * gQuadTrees[selectedGlobalIdx].Luminance / gQuadTrees[globalIdx].Luminance;
        localIdx = selectedLocalIdx;
    }

    return NAN;
}

float3 EvaluateQuadTree(uint quadTreeRootGlobalIdx, uint localIdx, out float pdf)
{
}

void RecordPoint(float3 position, float3 incomingRadiance, float3 wi)
{
    // TODO: Locks, Interlocked operations or separate buffer

    int quadTreeRootLocalIdx = FindQuadTreeIdx(position);
    if (quadTreeRootLocalIdx == -1)
        return;

    uint quadTreeRootGlobalIdx = GetQuadTreeRootGlobalIndex(quadTreeRootLocalIdx);

    float2 cy = SphereToCylinder(wi);

    float2 min = float2(COS_THETA_MIN, PHI_MIN);
    float2 max = float2(COS_THETA_MAX, PHI_MAX);

    uint localIdx = 0;
    while (localIdx < QUAD_TREE_MAX_NODES)
    {
        uint globalIdx = quadTreeRootGlobalIdx + localIdx;

        if (gQuadTrees[globalIdx].IsLeaf)
        {
            // TODO: Need to update all luminances up the tree hierarchy 

            float N = gQuadTrees[globalIdx].SampleCount;
            float lum = Luminance(incomingRadiance);
            float mean = (gQuadTrees[globalIdx].Luminance * N + lum) / (N+1);

            gQuadTrees[globalIdx].Luminance = mean; 
            gQuadTrees[globalIdx].SampleCount++;  
            return;
        }

        uint localIdx_NW = 4 * localIdx + 1;
        uint localIdx_NE = 4 * localIdx + 2;
        uint localIdx_SE = 4 * localIdx + 3;
        uint localIdx_SW = 4 * localIdx + 4;

        float2 mid = lerp(min, max, 0.5f);
        
        if (cy.x < mid.x)  // West
        {
            if (cy.y < mid.y) // South
            {
                localIdx = localIdx_SW;
                max = mid; 
            }
            else              // North
            {
                localIdx = localIdx_NW;
                min.y = mid.y; 
                max.x = mid.x; 
            }
        }
        else               // East
        {
            if (cy.y < mid.y) // South
            {
                localIdx = localIdx_SE;
                min.x = mid.x; // X-Up
                max.y = mid.y; // Y-Down
            }
            else              // North
            {
                localIdx = localIdx_NE;
                min = mid; 
            }
        }
    }
}

void RecordPath(VertexList vList)
{
    // TODO: Locks, Interlocked operations or separate buffer

    if (vList.NumVertices < 2)
        return;

    PathVertex vEnd = vList.Array[vList.NumVertices - 1];
    if (vEnd.Type != PathVertexType::eEnvironment && vEnd.Type != PathVertexType::eEmissive)
        return;

    if (length(vEnd.DirectContribution) == 0.0f)
        return;
    
    float3 Li = vEnd.DirectContribution;

    for (int i = vList.NumVertices - 2; i >= 0; i--)
    {
        PathVertex v1 = vList.Array[i];

        // Note: v1.IndirectContribution = bxdfContrib * NdL / bxdfPDF

        RecordPoint(v1.Position, Li, v1.Wi);

        Li *= v1.IndirectContribution;
    }
}

// TODO: Tree subdividing and pruning, how do they interact with recording?

// TODO: Change tree structure to use index pointers, otherwise memory use is so high it's pointless to have a tree
```