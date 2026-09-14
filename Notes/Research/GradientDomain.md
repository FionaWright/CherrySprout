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

# GD-PT Model

PT()  
    Trace()  
        # Generate ordinary path  
    SampleGradient()  
        for (dx and dy):  
            ShiftPrimaryRay()   
                # Shift by epsilon in one axis  
            ReplayPath()   
                # Use correlated path vertices  
                ShiftMapping()  
            EvaluateGradient()  

ReconstructGradientPoisson()

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