# Gradient Domain Rendering

Wiki:
    https://en.wikipedia.org/wiki/Gradient-domain_image_processing 
    https://en.wikipedia.org/wiki/Poisson's_equation

Original Paper:
    https://dl.acm.org/doi/10.1145/2766997

Students Paper:
    https://studenttheses.uu.nl/bitstreams/6de1ae1e-69bf-4488-b7f8-1ca9f922da44/download

Small-GDPT:
    https://gist.github.com/BachiLi/4f5c6e5a4fef5773dab1

SPR:
    https://www.ipol.im/pub/art/2014/84/revisions/2022-01-01/article.pdf

SPR Fourier Analysis:
    https://grail.cs.washington.edu/projects/screenedPoissonEq/

Code:
    https://github.com/gradientpm/gradient-mts
    https://github.com/mmanzi/gradientdomain-mitsuba

ReSTIR + GD:
    https://onlinelibrary.wiley.com/doi/full/10.1111/cgf.70328
    https://research.nvidia.com/labs/rtr/publication/wang2026gradient/

# Intro

Also called Poisson Image Editing

A type of image processing that operates on the difference between neighbouring pixel integrals rather than the final pixel values 

An image gradient represents the derivative of an image
The goal is to construct a new image by integrating the gradient using Poisson's Equation 

GD Image-Processing:
    Image -> Gradient -> Constraints -> Image

GD-PT:
    Light Transport -> Radiance Diffs -> Gradient Estimates -> Image

Gradients are estimated by integrating the difference between pairs of paths that are carefully sampled in a highly correlated fashion 
This is done by using shift mappings that generate pair paths that are similar so that differences are small and result in lower variance 

GD works best for functions with a frequency falloff (less energy in high frequencies) which is typical for light transport integrands 

Poisson reconstruction is used to combine gradients and pixel information to get the best of both worlds by relying on regular rendering for low frequencies and GD for high frequencies 

PT: $I(x) = \int f(x, p) \ dp$ 

GD-PT: $\nabla I(x) = \nabla_x \int f(x, p) \ dp$   
Differentiates a pixel with respect to an image space parameter $x$ 

Rather than differentiating the actual code, it computes a shifted correlated path

Gradient Estimator: $\dfrac{I(x + \Delta x) - I(x)}{\Delta x}$

For each vertex in a path, track:
```cpp
struct PathVertex
{
    float3 Position;
    float3 Normal;
    float3 Wo;
    float3 Wi;
    
    float PdfForward;
    float PdfReverse;

    bool IsDelta;
};
```

You also might need a `float PathPDF` for the total PDF of the path

The Gradient is just the difference between the main and shifted path contribution. I do not need to track anything different in the main path state (Except maybe PDFs ?)

Uniform sampling introduces uniform white noise at all frequencies with a dirac at the "DC" frequency (0)

The variable used by SPR to determine how much to take from the main sample vs the gradients is $\alpha$. The optimal value for $\alpha_{*}$ is different for each frequency, but MAY be able to be derived from the PT samples? Not sure

# GD-PT Model

PT()  
    Trace()  
        # Generate ordinary path  
    SampleGradient()  
        for (dx and dy):  
            ShiftPrimaryRay()   
                # Shift to a neighbouring pixel
            TraceShifted(MainPathVertices)   
                # Use correlated path vertices  
                ShiftMapping()  
            gradient = f_main - f_shifted

ScreenedPoissonReconstruction(primal, gx, gy)

# Begin

Note that we do NOT shift by epsilon, we use neighbouring pixels

```cpp
void GD_PT(pixelCoord)
{
    PathSample pathSample = Trace(pixelCoord);
    Accumulate(gPrimalTex, pixelCoord, pathSample.Primal);

    float3 gradient = 0;
    for (int j = 0; j < gSettings.NumGradients; j++)
    {
        NeighbourInfo neighbour = GetNeighbour(pixelCoord, j);
        float3 neighbourContribution = GradientMIS(neighbour) * TraceShifted(neighbour, pathSample.VerticesList); // Multplies by shift mapping jacobian internally

        gradient += pathSample.Primal - neighbourContribution;
    }

    gradient /= gSettings.NumGradients;

    gGradientTex[pixelCoord] = gradient;
}

// Seperate CS passes. Iterative Jacobi for base implementation. Or FFT might be better 
// Pseudocode not exactly matching what happens. But gives general idea of inputs/outputs
void Reconstruct()
{
    float3 primal = gPrimalTex[pixelCoord];
    float3 gradient = gGradientTex[pixelCoord];

    float3 poisson = ScreenedPoissonReconstruct(primal, gradient, gSettings.GradientAlpha);

    gOutputTex[pixelCoord] = poisson;
}
```

```cpp
bool IsSymmetric(PathSample main, PathSample shifted)
{
    if (!PossibleMainSample(shifted)) // Can be sampled by the path-tracer
        return false; // w_ij == 1, w_ji == 1

    if (!InverseMappingExists(main, shifted)) // For shift mapping T_ij, T_ji exists
        return false; // w_ij == 1, w_ji == 0

    // If for a vertex, the main path does a refraction whereas the shifted path is forced into TIR, the reverse shift will still be a reflection. Thus it is uninvertable.
    // TODO: Explain better

    // If when attempting a reconnection shift (into a diffuse vertex), it is occluded, then the path is not symmetric 

    return true;
}

// Note: NEE Emissive light sources make the MIS more complex 
float GradientMIS(PathSample main, PathSample shifted)
{
    if (!IsSymmetric(main, shifted))
        return 1.0f;

    float p1 = main.PDF;
    float p2 = shifted.PDF * shifted.JacobianDet;
    return BalanceHeuristic(p1, p2); // p1 / (p1 + p2)
}
```

```cpp
PathSample TraceShifted(VertexList vertices)
{
    bool reconnected = false;
    for (uint v = 0; v < vertices.Count; v++)
    {
        Vertex vertex = vertices.Array[v];
        Vertex nextVertex = vertices.Array[v+1];

        if (reconnected)
        {
            TraceToVertex(vertex);
        }
        else if (!vertex.IsDirac && !nextVertex.IsDirac)
        {
            ReconnectionShift();
            VisibilityTest(nextVertex);
            reconnected = true;
        }
        else
        {
            HalfVectorShift();
            TraceNextRay();
        }
    }
}
```

## Jacobian

### For Half-Vector Shifts:

Jacobian Determinent for reflection is:  
$|T'_{ij}| = \dfrac{w_o^y \cdot h^y}{w_o^x \cdot h^x}$  

And for refraction:  
$\eta^x = \dfrac{n_2^x}{n_1^x}$  
$|T'_{ij}| = \dfrac{|w_i^y + \eta^y w_o^y|^2}{|w_i^x + \eta^x w_o^x|^2} \dfrac{w_i^x \cdot h^x}{w_i^y \cdot h^y}$  

### For the Reconnection Shift:

$x_1$ is the current vertex   
$x_2$ is the next vertex ($x_2^x = x_2^y$ right?)  
$cos(\theta^x)$ is dot(surfaceNormal, $x_1 \to x_2$)  

$|T'_{ij}| = \dfrac{cos(\theta^y)}{cos(\theta^x)} \dfrac{|x_1^x - x_2^x|^2}{|x_1^y - x_2^y|^2}$  

If connecting to the env map, $|T'_{ij}| = 1$ 

# Screened Poisson Reconstruction

Will use a type of DFT called Discrete Cosine Transform (DCT). Uses cosines only

$f$ = The integral to solve for 
$g^x$ = The Gradient Sample (X)  
$g^y$ = The Gradient Sample (Y)  
$u$ = The primal sample, used as a constraint
$\lambda_d$ = $\alpha$  
$d_x$ = Discrete Derivative Filter (X) ?  
$d_y$ = Discrete Derivative Filter (Y) ?  

$\dfrac{\partial f}{\partial x} = d_x \circ f$   
$\dfrac{\partial}{\partial x} \dfrac{\partial f}{\partial x} = d_x \circ d_x \circ f$  

Apply DFT on the variables to find them in the fourier domain (Capital letters)  

Spatial $\circ \to$ Fourier $\times$

$\lambda_d F = D_x^2 F - D_x^2 F = \lambda_d U - D_x G^x - D_y G^y$  

$F = \dfrac{\lambda_d U - D_x G^x - D_y G^y}{\lambda_d - D^2_x - D^2_y}$  

"Typical choices for these discrete derivatives are forward, backward, or central differences"

The numerator can be converted to fourier space in one pass:  
$h = \lambda_d u - d_x \circ g^x - d_y \circ g^y$  
Then compute $H$  

For continuous problems (not what is needed):
    $D_x = 2i \pi s_x$ (Bracewell Notation)  
    $D_x^2 = -4 \pi^2 i s_x^2$ 
    Solution is undefined at $s_x = s_y = 0$. Constant offset (DC term) must be supplied. UNLESS $\alpha > 0$ in which case $F(0,0) = U(0,0)$ (Spatial Frequency are the parameters?)

DFT is worse than DCT as it assumes the input sequence is periodic which it isn't in this case  
DCT performs reflections across boundaries before tiling the plane periodically 

The paper mentions computing the gradients using backward differences and the derivative filters are forward differences (when computing h)

They say they computed the DCT using "brute force" without explicitly storing it (May not be relevant)

See FFTW library for DCT conversion source code (CPU only). Computes transforms in O(NlogN)

## Code First Draft

```cpp
// Pseudocode

// Operates over whole texture, computes forward difference between pixels
float DiscreteDerivativeX(float gradientX)
{
    // ?
}

float DiscreteDerivativeY(float gradientY)
{

}

float Dx2(int frequency, int imageWidth)
{
    // input frequency
    // Code is different depending on DiscreteDerivativeX()

    // Keywords: Eigenvalues, Reflected/Neumann DCT formulation
}

float Dy2(int frequency, int imageHeight)
{

}

// Spatial-space textures -> fourier-space textures
// Note: May be multiple passes/iterations
void SPR_FirstPass(float3 primal, float3 gradient, float alpha)
{
    // Do I use float gradient.x, gradient.y or float3 gradientX, gradientY?
    float dxgx = DiscreteDerivativeX(gradient.x);
    float dygy = DiscreteDerivativeY(gradient.y);

    float3 h = alpha * primal - dxgx - dygy;
    FFT_TransformToDCT(gTexH, h);
}

// Perform operation on the fourier-space textures
void SPR_SecondPass(uint2 frequencyCoord, float alpha)
{
    float3 H = gTexH[frequencyCoord];

    // These are eigenvalues?
    float Dx2 = Dx2(frequencyCoord.x, IMAGE_WIDTH);
    float Dy2 = Dy2(frequencyCoord.y, IMAGE_HEIGHT);

    float3 F = H / (alpha - Dx2 - Dy2);
    
    gTexF[frequencyCoord] = F;
}

// Fourier-space textures -> spatial-space textures
// Note: May be multiple passes/iterations
float3 SPR_ThirdPass(uint2 pixelCoord)
{
    return FFT_TransformFromDCT(gTexF);
}
```

## Code Second Draft

```cpp

void SPR_ComputeHSpatial(uint2 pixelCoord, float alpha)
{
    float3 gradient = gTexGradient[pixelCoord];
    float3 gradientX = gTexGradient[pixelCoord + uint2(1,0)];
    float3 gradientY = gTexGradient[pixelCoord + uint2(0,1)];

    // Forward Difference
    float3 dxgx = gradientX - gradient;
    float3 dygy = gradientY - gradient;

    float3 primal = gTexPrimal[pixelCoord];
    
    float3 h = alpha * primal - dxgx - dygy;
    gTexHSpatial[pixelCoord] = h;
}

void SPR_ComputeHFourier()
{
    // Apply FFT (DCT) to convert gTexHSpatial to gTexHFourier 
}

void SPR_Core(uint2 frequencyCoord, float alpha)
{
    float3 H = gTexHFourier[frequencyCoord];

    float Dx2 = Dx2(frequencyCoord.x, IMAGE_WIDTH);
    float Dy2 = Dy2(frequencyCoord.y, IMAGE_HEIGHT);

    float3 F = H / (alpha - Dx2 - Dy2);
    
    gTexFFourier[frequencyCoord] = F;
}

void SPR_ComputeFSpatial()
{
    // Apply Inverse-FFT (DCT) to convert gTexFFourier to gTexFSpatial
}

```