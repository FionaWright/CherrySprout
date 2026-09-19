# Jacobi SPR

## Math

$f_x = \dfrac{\partial f}{\partial x}$  

$f_{x_i} = f_i$  

$g : \mathbb{R}^n \mapsto \mathbb{R}^m$  
$g(x_1, x_2, \dots) = (y_1, y_2, \dots)$  
$g^i : \mathbb{R}^n \mapsto \mathbb{R}$  
$g^3(x_1, x_2, \dots) = y_3$  

$f \in \mathbb{R}^n$  
$f^2 = x_1^2 + x_2^2 + \dots + x_n^2$  

### Higher-order functions / Functionals

Functions where the domain is $\mathcal{F}$  
Takes functions as inputs  

Includes derivative, integral, nabla, etc

### Jacobian Matrix

$g : \mathbb{R}^n \mapsto \mathbb{R}^m$  
$J_g = \begin{bmatrix} \nabla^T g_1 \\ \dots \\ \nabla^T g_m \end{bmatrix}$  

### Hessian Matrix

$f : \mathbb{R}^n \mapsto \mathbb{R}$  

$H_f = \begin{bmatrix} \dfrac{\partial^2 f}{\partial x_1^2} & \dfrac{\partial^2 f}{\partial x_1 \partial x_2} & \dots & \dfrac{\partial^2 f}{\partial x_1 \partial x_n} \\ \dfrac{\partial^2 f}{\partial x_2 \partial x_1} & \dfrac{\partial^2 f}{\partial x_2^2} & \dots & \dfrac{\partial^2 f}{\partial x_2 \partial x_n} \\ \dots & \dots & \dots & \dots \\ \dfrac{\partial^2 f}{\partial x_n \partial x_1} & \dfrac{\partial^2 f}{\partial x_n \partial x_2} & \dots & \dfrac{\partial^2 f}{\partial x_n^2} \end{bmatrix}$  

### Nabla

A vector of partial derivatives for each input  

$\nabla : \mathcal{F} \mapsto \mathcal{F}^n$  
$f : \mathbb{R}^n \mapsto \mathbb{R}$

$\nabla f = (\dfrac{\partial f}{\partial x}, \dfrac{\partial f}{\partial y}, \dfrac{\partial f}{\partial z}) = [\dfrac{\partial f}{\partial x}, \dfrac{\partial f}{\partial y}, \dfrac{\partial f}{\partial z}]^T$  

$\nabla^T f = [\dfrac{\partial f}{\partial x}, \dfrac{\partial f}{\partial y}, \dfrac{\partial f}{\partial z}]$  

$A$ is a matrix  
$trace(A)$ is the sum of diagonal elements  

### Nabla Dot

A higher order function that returns the sum of partial derivatives 

$\nabla \cdot : \mathcal{F}^n \mapsto \mathcal{F}$  
$g : \mathbb{R}^n \mapsto \mathbb{R}^m$  

$\nabla \cdot g = div(g) = \sum \dfrac{\partial g^i}{\partial x_i}$  

### Laplacian

Returns the sum of second-order partial derivatives for each of the inputs

$\Delta : \mathcal{F} \mapsto \mathcal{F}$  
$f : \mathbb{R}^n \mapsto \mathbb{R}$  

$\Delta f = \nabla \cdot \nabla f = \sum \dfrac{\partial^2 f}{\partial^2 x_i} = trace(H_f)$  

## Forming the equation

$\underset{u}{min} \int \alpha (u - f)^2 - (\nabla u - g)^2 \ dx$  

Find min $u$ for this equation

$x$ = Pixel Coordinates  
$\alpha$ = Tuning Parameter  
$u(x)$ = The Solved Function  
$f(x)$ = The Primal  
$g(x)$ = The Gradients  

Ignore RGB channels, assume all equations are applied across all channels individually with no dependencies  

$u : \mathbb{R}^2 \mapsto \mathbb{R}$  
$f : \mathbb{R}^2 \mapsto \mathbb{R}$  
$g : \mathbb{R}^2 \mapsto \mathbb{R}^2$  

### The Lagrange Equation
$\dfrac{\partial L}{\partial u} - \sum \dfrac{\partial}{\partial x_i} \dfrac{\partial L}{\partial u_i} = 0$  

$L(x, u, \nabla u) = \alpha (u - f)^2 - (\nabla u - g)^2$  

### Solving for $\frac{\partial L}{\partial u}$

$\dfrac{\partial L}{\partial u} = 2 \alpha (u - f)$  

### Solving for $\frac{\partial L}{\partial u_i}$

$\dfrac{\partial L}{\partial u_i} = \dfrac{\partial}{\partial u_i} (\nabla u - g)^2$   

$\nabla u = (\dfrac{\partial u}{\partial x_1}, \dfrac{\partial u}{\partial x_2})$  

$g = (g^1, g^2)$  

$\dfrac{\partial L}{\partial u_i} = \dfrac{\partial}{\partial u_i} ((\dfrac{\partial u}{\partial x_1}, \dfrac{\partial u}{\partial x_2}) - (g^1, g^2))^2$   

$\dfrac{\partial L}{\partial u_i} = \dfrac{\partial}{\partial u_i} ((\dfrac{\partial u}{\partial x_1} - g^1, \dfrac{\partial u}{\partial x_2}) - g^2)^2$   

$\dfrac{\partial L}{\partial u_i} = \dfrac{\partial}{\partial u_i} (\dfrac{\partial u}{\partial x_1} - g^1)^2 + (\dfrac{\partial u}{\partial x_2} - g^2)^2$   

$\dfrac{\partial L}{\partial u_i} = \dfrac{\partial}{\partial u_i} (\sum (\dfrac{\partial u}{\partial x_i} - g^i)^2)$   

$\dfrac{\partial L}{\partial u_i} = \dfrac{\partial}{\partial u_i} (\sum (u_i - g^i)^2)$   

$\dfrac{\partial L}{\partial u_i} = 2(u_i - g^i)$  

### Solving for $\frac{\partial}{\partial x_i} \frac{\partial L}{\partial u_i}$ 

$\dfrac{\partial}{\partial x_i} \dfrac{\partial L}{\partial u_i} = \dfrac{\partial}{\partial x_i} 2(u_i - g^i)$  

$\dfrac{\partial}{\partial x_i} \dfrac{\partial L}{\partial u_i} = 2(\dfrac{\partial u_{i}}{\partial x_i} - \dfrac{\partial g^i}{\partial x_i})$  

$\dfrac{\partial}{\partial x_i} \dfrac{\partial L}{\partial u_i} = 2(u_{ii} - g^i_i)$  

### Substitution

$2 \alpha (u-f) - \sum 2(u_{ii} - \dfrac{\partial g^i}{\partial x_i}) = 0$  

$\alpha (u-f) - \sum (u_{ii} - \dfrac{\partial g^i}{\partial x_i}) = 0$  

$\alpha (u-f) - \sum u_{ii} + \sum \dfrac{\partial g^i}{\partial x_i} = 0$  

$\sum u_{ii} = \Delta u$  
$\sum \dfrac{\partial g^i}{\partial x_i} = \nabla \cdot g$  

$\alpha (u-f) - \Delta u + \nabla \cdot g = 0$ 

## Linear System

All operators are linear, can be put into form:  
$Ax + b = 0$  

$\alpha u- \alpha f - \Delta u + \nabla \cdot g = 0$  

$(\alpha - \Delta)u - \alpha f + \nabla \cdot g = 0$  

$A$ is a matrix containing the operations of $\alpha I - \Delta$  

$b = - \alpha f + div(g)$  

Gradients were computed forwards, divergence must be computed backwards for quantization to cancel out  

$\Delta u = u_{i+1,j} + u_{i,j+1} + u_{i-1,j} + u_{i,j-1} - 4 u_{i,j}$  

Given $n \times m = N$ pixels that can be laid out in a vector:  
$(x_{1,1}, x_{1,2}, \dots, x_{1,n}, x_{2,1}, \dots, x_{m,n})$  

Let $n = m = 3$  

$\Delta \in \mathbb{R}^N$  

$\Delta = \begin{bmatrix} 
-4 & 1 & 0 & 1 & 0 & 0 & 0 & 0 & 0 \\
1 & -4 & 1 & 0 & 1 & 0 & 0 & 0 & 0 \\
0 & 1 & -4 & 0 & 0 & 1 & 0 & 0 & 0 \\
1 & 0 & 0 & -4 & 1 & 0 & 1 & 0 & 0 \\
0 & 1 & 0 & 1 & -4 & 1 & 0 & 1 & 0 \\
0 & 0 & 1 & 0 & 1 & -4 & 0 & 0 & 1 \\
0 & 0 & 0 & 1 & 0 & 0 & -4 & 1 & 0 \\
0 & 0 & 0 & 0 & 1 & 0 & 1 & -4 & 1 \\
0 & 0 & 0 & 0 & 0 & 1 & 0 & 1 & -4 \end{bmatrix}$  

$\alpha I - \Delta = \begin{bmatrix} 
(\alpha + 4) & -1 & 0 & -1 & 0 & 0 & 0 & 0 & 0 \\
-1 & (\alpha + 4) & -1 & 0 & -1 & 0 & 0 & 0 & 0 \\
0 & -1 & (\alpha + 4) & 0 & 0 & -1 & 0 & 0 & 0 \\
-1 & 0 & 0 & (\alpha + 4) & -1 & 0 & -1 & 0 & 0 \\
0 & -1 & 0 & -1 & (\alpha + 4) & -1 & 0 & -1 & 0 \\
0 & 0 & -1 & 0 & -1 & (\alpha + 4) & 0 & 0 & -1 \\
0 & 0 & 0 & -1 & 0 & 0 & (\alpha + 4) & -1 & 0 \\
0 & 0 & 0 & 0 & -1 & 0 & -1 & (\alpha + 4) & -1 \\
0 & 0 & 0 & 0 & 0 & -1 & 0 & -1 & (\alpha + 4) \end{bmatrix}$  

### Jacobi

For some functions where:  
$f(x) = x$  

$\underset{n \to \infty}{lim} \  z^{(n)} = x$  
Where $z^{(k+1)} = f(z^{(k)})$  

$Au + b = 0$  
$Au + u + b = u$  
$(A + I)u + b = u$  

### Optimization

Undo the jacobi part 

Let $A = L + U + D$  

$D$ is the diagonal elements of $A$  
$L + U$ are the non-diagonal elements

$(L + U + D)u + b = 0$  
$D^{-1}(L + U + D)u + D^{-1}b = 0$  
$D^{-1}(L + U)u + D^{-1}Du + D^{-1}b = 0$  
$D^{-1}(L + U)u + u + D^{-1}b$  
$D^{-1}((L + U)u + b) = -u$  
$-D^{-1}((L + U)u + b) = u$  

$L + U = \begin{bmatrix} 
0 & -1 & 0 & -1 & 0 & 0 & 0 & 0 & 0 \\
-1 & 0 & -1 & 0 & -1 & 0 & 0 & 0 & 0 \\
0 & -1 & 0 & 0 & 0 & -1 & 0 & 0 & 0 \\
-1 & 0 & 0 & 0 & -1 & 0 & -1 & 0 & 0 \\
0 & -1 & 0 & -1 & 0 & -1 & 0 & -1 & 0 \\
0 & 0 & -1 & 0 & -1 & 0 & 0 & 0 & -1 \\
0 & 0 & 0 & -1 & 0 & 0 & 0 & -1 & 0 \\
0 & 0 & 0 & 0 & -1 & 0 & -1 & 0 & -1 \\
0 & 0 & 0 & 0 & 0 & -1 & 0 & -1 & 0 \end{bmatrix}$  

$L + U \equiv -u_{i-1, j} - u_{i+1, j} - u_{i,j+1} - u_{i,j-1}$  

$D = \begin{bmatrix} 
(\alpha + 4) & 0 & 0 & 0 & 0 & 0 & 0 & 0 & 0 \\
0 & (\alpha + 4) & 0 & 0 & 0 & 0 & 0 & 0 & 0 \\
0 & 0 & (\alpha + 4) & 0 & 0 & 0 & 0 & 0 & 0 \\
0 & 0 & 0 & (\alpha + 4) & 0 & 0 & 0 & 0 & 0 \\
0 & 0 & 0 & 0 & (\alpha + 4) & 0 & 0 & 0 & 0 \\
0 & 0 & 0 & 0 & 0 & (\alpha + 4) & 0 & 0 & 0 \\
0 & 0 & 0 & 0 & 0 & 0 & (\alpha + 4) & 0 & 0 \\
0 & 0 & 0 & 0 & 0 & 0 & 0 & (\alpha + 4) & 0 \\
0 & 0 & 0 & 0 & 0 & 0 & 0 & 0 & (\alpha + 4) \end{bmatrix}$  

For a diagonal matrix $D$, $D^{-1}$ just has all the elements inverted 

$D^{-1} \equiv \dfrac{1}{\alpha + 4}$  

$u^{(k+1)} = -\dfrac{1}{\alpha + 4} (-u_{i-1, j}^{(k)} - u_{i+1, j}^{(k)} - u_{i,j+1}^{(k)} - u_{i,j-1}^{(k)} + b)$

Due to mirror boundary conditions when a pixel is on the edge, for example when $u_{i+1,j}$ is OOB then:    
$u_{i+1,j} = u_{i,j}$   
$\Delta u = 0 + u_{i-1,j} + u_{i,j+1} + u_{i,j-1} - 3 u_{i,j}$  

Thus you should sample zero for each OOB pixel and reduce the 4 coefficient by 1