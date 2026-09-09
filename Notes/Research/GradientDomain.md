# Gradient Domain Rendering

Wiki:
    https://en.wikipedia.org/wiki/Gradient-domain_image_processing 
    https://en.wikipedia.org/wiki/Poisson's_equation

Original Paper:
    https://dl.acm.org/doi/10.1145/2766997

Code:
    https://github.com/gradientpm/gradient-mts

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