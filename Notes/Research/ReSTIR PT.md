# ReSTIR PT

Given all subpaths from NEE / BXDF / PG sampling generated within a single path sample, candidate samples are a single selected subpath

If you let light vertices only be sampled by NEE, then the ReSTIR MIS weight is 1

Given path sample of depth $D$, vertices $\{x_0, x_1, x_2, \dots, x_{D-1}\}$ 

NEE subpath is $\{x_0, x_1, \dots x_k\}$ 

Target function is just contribution

If BxDF can sample lights, now both can be used as candidate paths. Each $m_i(x_i)$ accounts for the other sampling technique of the same depth

I think the main difference between GI and PT is that GI only stores the next vertex, and if you fail to connect then give up
Whereas PT stores a vertex list, and the path traces a full shifted with reconnection states?

## GDPT Shift Mapping

Identity shift?
An area measure jacobian

$T[x_0, x_1, x_2, \dots, x_{D-1}] = [y_0, y_1, x_2, \dots, x_{D-1}]$ 

Looks a lot like the reconnection shift but idk
Should be ReconnectVis()

Can use the same shift mappings from GDPT

NOTE: It mentions that when v1, v2 are diffuse and you are ready to do a reconnection shift. The current y vertex CANNOT be specular otherwise the shift fails. Does this mean I need to detect whether the vertex is glossy before shift mapping? Surely that doesn't involve the BxDF so maybe just check roughness/metallic

GRIS paper has lots of info on local shift mappings apparently. There's also global methods like manifold exploration which solve specular chains

Also see "A survey on gradient domain rendering 2019" 

## Modified ReSTIR Shift Mapping (Hybrid Shift)

Similarly postpones reconnection as GDPT SM

Random-Replay Shift = Copy the rng state at each bounce and re-trace the next bounce with the method used by the base path

Precomputes the reconnection vertex on the base path and uses random replay to shift earlier path segments so that the base path does not have to be stored 

Uses additional distance condition similar to Manzi et al 2014, to avoid creating short reconnection segments 

Classifies a vertex using ONLY roughness of the sampled lobe, optimizing resampling on multi-layer materials 

Requires constant storage per pixel, only a reconnection vertex and an RNG generating seed in addition to other reservoir data 

This can significantly improve the quality of glossy/refractive materials compared to reconnection shift 

### Ensuring Invertibility

To be connectible, a reconnection vertex needs to satisfy two conditions:

Distance Condition:
```cpp
float dX = LengthSquared(x2 - x1);
float dY = LengthSquared(x2 - y1);
if (dX >= D_THRESHOLD && dY >= D_THRESHOLD)
{
    Reconnect();
}
```

Roughness Condition:
```cpp
float alpha1 = Lobe(x1).Alpha;
float alpha2 = Lobe(x2).Alpha;
float alphaY = Lobe(y1).Alpha;
if (alpha1 >= ALPHA_THRESHOLD && 
    alpha2 >= ALPHA_THRESHOLD && 
    alphaY >= ALPHA_THRESHOLD)
{
    Reconnect();
}
```

If a vertex contains multiple lobes (If it samples lights for NEE and evaluates all lobes) then pick the lobe that maximizes the alpha

When tracing the base path, the first vertex (That isn't the primary vertex) that satisfies both conditions is marked as the reconnection vertex

When the shifted path runs, the offset path is generated using random replay until it reaches the reconnection vertex index. Then it checks both conditions 

I'd recommend trying to implement hybrid shift into GDPT before starting ReSTIR PT

### Jacobian

Random-Replay shift has no/identity jacobian. But hybrid shift has a jacobian on the reconnection

$x_k$ is the reconnection vertex (Not the final vertex)

$\mathcal{p}_{k-1}(x_k)$ is the probability of selecting the lobe $\mathcal{l}_{k-1}$ multiplied by the probability of sampling direction $\omega_{k-1}$ on path $x$ 

If $x_k$ is a regular vertex:

$J = \dfrac{\mathcal{p}_{k-1}(y_k)}{\mathcal{p}_{k-1}(x_k)} \dfrac{\mathcal{p}_{k} (y_{k+1})}{\mathcal{p}_{k}(x_{k+1})} J_R$

$J_R$ is the reconnection shift jacobian

If $x_k$ is an NEE vertex:

$J = \dfrac{\mathcal{p}_{k-1}(y_k)}{\mathcal{p}_{k-1}(x_k)} J_R$

Note that the lobes and directions will always be the same between paths assuming the scene is static. But their sampling PDFs will be different due to different outgoing directions towards the previous path vertices

If $x_k$ is a light vertex sampled by NEE then $p_{k-1}(y_k)$ is just the light sampling PDF, ignoring the lobe sampling PDF

The denominator relating to the $x$ path can be cached in the candidate data to avoid re-computation

## Lobe Tags

Each vertex gets assigned a lobe tag on the main path

NEE is its own sampling technique and as such gets its own tag. This tag can only be applied on the final vertex! (Unless you consider the NEE hit vertex as the last vertex)

## Struct

```cpp
struct ReconnectionVertexInfo
{
    float3 Wi;
    float3 Lo;
    uint GlobalPrimitiveOffset; // To ID triangle
    float2 Bary;
    LobeTag LobePrev;
    LobeTag Lobe;
};

struct CandidatePT
{
    ReconnectionVertexInfo RcVertexInfo;
    uint InitialRngState;
    uint PostReconnectRngState; // Only needed when scene is not static
    uint ReconnectVertexIndex;
    float JacobianDenominator;
};
```