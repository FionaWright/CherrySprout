# GPPT Jacobian

## Shifts

$f_s(x_s) = \tilde f_s(x_s) |J|$  
$p_s(x_s) = \tilde p_s(x_s) |J|$  

$p_s = \tilde p_s |J|$ 

$\tilde f_s(x_s)$ internally divided by $\tilde p_s(x_s)$ 

## PDF Ratio

$N$ = Number of vertices  

$p_m = p_{m,0} p_{m,1} \dots p_{m,N}$  
$\tilde p_s = \tilde p_{s,0} \tilde p_{s,1} \dots \tilde p_{s,N}$  

$p_R = p_s / p_m$  

Assume full connection at vertex $i$  

$p_R = \dfrac{p_{s,0} p_{s,1} \dots p_{s,i-1}}{p_{m,0} p_{m,1} \dots p_{m,i-1}}$ 

## MIS Weight

$w = \dfrac{p_m}{p_m + p_s}$

$w = \dfrac{1}{1 + \dfrac{p_s}{p_m}}$ 

$w = \dfrac{1}{1 + \dfrac{\tilde p_s |J|}{p_m}}$ 

## Gradient

$G = (f_m(x_m) - f_s(x_s)) w$   
$G = (f_m(x_m) - \tilde f_s(x_s) |J|) w$ 

# With NEE

$p_{m,i} = p^{bxdf}_{m,i} p^{nee}_{m,i}$   
$p_m = \prod p_{m,i}$ 