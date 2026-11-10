# Gradient-Domain ReSTIR PT

You cannot run simply ReSTIR on GDPT primal+shifted pairs as the gradients can be negative wheras the target function needs to be positive  
This is solved using positivation (Separate reservoirs for positive and negative)

Area ReSTIR might be needed for this to work properly 

One shift mapping is used to maintain path correlations for finite difference computations, another is used for spatiotemporal reuse

Bad:  
    Apply ReSTIR PT to reuse paths  
    Then shift the final sample for GDPT  
    Since PDFs are lost post-ReSTIR and required for GDPT, $\hat p(x)$ and $\hat p(y)$ can be used for MIS  
    This oversamples areas where path domains overlap and undersamples areas where they differ (Areas that GDPT really wants to know about)  

Better:  
    Reuse primal-shifted pair paths  
    This samples the integral according to the path differences, so no undersampling   

In their method, they do a separate Area-ReSTIR pass to render the primal image 

Inner Shift ($T_{ij}$):
    The shift done by GDPT between the primal and shifted paths

Outer Shift ($T^*$):
    The shift done by ReSTIR for reuse

When reusing a sample, first do the outer shift to find the new primal path then apply the inner shift to find the shifted path

$k \in \{0,1\}$  
$k=0 \to \bar x \in \Omega_i$ (Forward)  
$k=1 \to \bar x \in \Omega_j$ (Backward)  
Shows whether using the forward or backwards estimator  

$\Delta_{ij} = W_{\bar y,k} \omega^*_{ij}(\bar y^*, k^*) G_{ij}(\bar y^*, k^*)$ 

$\hat p(x, k) = \omega^*_{ij} (T^*(\bar x, k)) |G_{ij}(T^*(\bar x, k))|$  
Using $|G_{ij}|$ is the naive approach however, should be replaced with positivation

$T^*(\bar x, k) = (\tau(\bar x), \gamma(k))$  

$\tau$ shifts the base path  
$\gamma$ shifts the indicator $k$  
$\gamma$ is the identity shift  

When computing area-restir fractional MVs for a pair-path, use the pixel with the smaller depth in the GBuffer as the pivot and use its fractional MV for the entire pair

For spatial reuse, image gradients can be sparse in the local neighborhood. So a new approach is needed:  
Adaptively use the GBuffer to determine whether spatial reuse is even useful, and perform edge-aligned spatial search and reuse  
If $|a(i) - a(j)| > 0.1, n(i) \cdot n(j) > 0.9, |d(i) - d(j)|/d(i) > 0.1$ then the pair is considered an edge pair. Only edge pairs perform spatial reuse  
$a$ is albedo  
$n$ is normal  
$d$ is depth  
For edge pairs, if any interior (non-edge) pairs are found they should be ignored and continue searching  

## Positivation

For a real-valued function, it can be split into positive and negative parts and estimated separately    
$f(x) = max(f(x), 0) - max(-f(x), 0)$  

$\Delta_{ij} = \int \hat p_+ (\bar x, k) d(\bar x, k) - \int \hat p_-(\bar x, k) d(\bar x, k)$  

$\hat p_+ = max(\hat p, 0)$  
$\hat p_- = max(-\hat p, 0)$   

For generating initial candidates, they all go into both the postive and negative reservoir   
When reusing samples, only reuse from the same signed reservoir as the canonical sample  
Reusing samples across signs is a future work  

There's a reservoir for each pixel pair and for each +/-  
Forward and backward estimators share the same reservoir however  
So total reservoirs:  
X+, X-, Y+, Y-  

## Questions

Assuming forward/backward share the same reservoir buffer, how are candidates generated? Pick forward/backward randomly? 

When a primal path is generated, is it okay to use it for both an X and Y gradient? Or will that introduce correlation that we don't want?  
Or does the X and Y difference actually exist as a single candidate?