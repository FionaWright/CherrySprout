# Gradient Domain Rendering

Wiki:
    https://en.wikipedia.org/wiki/Gradient-domain_image_processing 
    https://en.wikipedia.org/wiki/Poisson's_equation

Original Paper:
    https://dl.acm.org/doi/10.1145/2766997

SPR:
    https://www.ipol.im/pub/art/2014/84/revisions/2022-01-01/article.pdf

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

// Seperate CS passes. Iterative Jacobi for base implementation 
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
void GradientMIS(PathSample main, PathSample shifted)
{
    float p1 = main.PDF;

    float p2 = ShiftMappingJacobian(main, shifted) * JacobianDet(main, shifted);

    return BalanceHeuristic(p1, p2); // p1 / (p1 + p2)
}
```

# Screened Poisson Reconstruction

A way to recover an image from its desired gradients while optionally having constraints 

$f$ = Unknown reconstructed image  
$v$ = Gradient field  
$f_0$ = Reference image (primal)   
$\lambda$ = Screening Strength  

$\underset{f}{min} \int || \nabla f - v ||^2 \ dx + \lambda \int (f - f_0)^2 \ dx$

The first term makes the gradients of $f$ match $v$   
The second term prevents $f$ from drifting too far from $f_0$  

Ordinary poisson reconstruction isn't enough (Find $\nabla f = v$) as the gradients have been measured with noise  
So instead we find the closest integrable gradient field in a least-squares sense 

For each pixel $p_{i,j}$ we sample $I^0_{i,j}, g_{i,j}^x, g_{i,j}^y$ 

The final equation finds $I_{i,j}$ by minimizing:

$\underset{I}{min} \sum (I_{i+1,j} - I_{i,j} - g_{i,j}^x)^2 + (I_{i,j+1}-I_{i,j}-g_{i,j}^y)^2 + \lambda (I_{i,j} - I_{i,j}^0)^2$  

This is a massive linear system across all pixels and cannot be solved independently for each pixel

Laplacian($I$) = divergence($g$) + $\lambda (I-I^0)$  
$\nabla^2 I - \lambda I = \nabla \cdot g - \lambda I^0$  

We have a linear system:  
$AI = b$  

$A = \begin{bmatrix} 4 + \lambda & -1 & 0 & \dots \\ -1 & 4 + \lambda & -1 & \dots \\ 0 & -1 & 4 + \lambda & \dots \\ \dots & \dots & \dots & \dots \end{bmatrix}$  

Note that the matrix is sparse and highly structured, we do not need to store it in memory and can use it implicitly 

## Jacobi Iterations

Iterative solvers are then used to solve the linear system, such as Jacobi Iteration which is GPU-friendly  

$(4 + \lambda) I_{i,j} = I_{i-1, j} + I_{i+1,j} + I_{i,j-1} + I_{i,j+1} + b_{i,j}$  

Where $b_{i,j} = \nabla g_{i,j} + \lambda I_{i,j}^0$   

Solve for $b_{i,j}$ and store it in a buffer, then use ping-pong textures to perform the iterations 

```cpp
CS(x,y):
    left = prev[x-1,y];
    right = prev[x+1,y];
    up = prev[x,y-1];
    down = prev[x,y+1];

    b = b[x,y];

    I_new = (left + down + up + right + b) / (4 + lambda);
```

Jacobi converges slow however. We can't use Gauss-Seidel as it breaks parallelism but we can use a clever trick:

## Red-Black Gauss Seidel

Split pixels into a checkerboard pattern where its either assigned as red or black. Update all reds in parallel and all blacks in parallel 

Allows for improved convergence without breaking parallelism 

## Conjugate Gradient / MultiGrid

Possibly even better solutions? Do the simpler ones then look into this