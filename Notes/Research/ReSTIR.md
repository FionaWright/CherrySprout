# ReSTIR

Notes primarily based on "A Gentle Introduction to ReSTIR" by Chris Wymann

Resevoir-based Spatio-Temporal Importance Sampling

Built upon RIS. Key idea is to re-use information between samples to find PDFs that are more proportional to the integrand, thus giving less variance

Can give 100x efficiency improvement if highly optimized

RIS and ReSTIR combine samples in a smarter way than a denoiser, as they filter/re-use/resample before throwing any information away. Plus they have access to intermediate values whereas denoisers only have the final color plus some guide GBuffers. 

ReSTIR works as a good path for a pixel with minor modifications is usually good for its neighbours. Radiance magnitude is our heuristic for which paths are "good". 

## Preliminaries 

For pixel $i$:  
$I_i = \int_{\Omega} h_i(x) f(x) \ dx$  
$h_i$ : Image filter  
$f$ : Measurement Contribution  
$dx$ : Product-Area Measure  

We will assume $h$ is a box filter (All paths through the pixel are averaged). Thus:  
$I_i = \int_{\Omega_i} f(x) \ dx$   
$\Omega_i$ : Paths that pass through pixel $i$  

Unbiased Monte Carlo Estimator for $I_i$:  
$\langle I_i \rangle = \dfrac{f(X)}{p(X)} \approx I_i$ 

Variance can be reduced by averaging more samples:  
$\langle I_i \rangle = \dfrac{1}{N} \overset{N}{\underset{j=1}{\sum}} \dfrac{f(X_j)}{p_j(X_j)}$

$p_j$ can be different for each sample

A uniform distribution corresponds to $p_j(X_j) = \dfrac{1}{|\Omega|}$ 

Each halving of noise magnitude requires 4x the samples. It's usually better to make the density $p$ match $f$. 

Uppercase = Random Variables
Lowercase = Traditional Variables

Estimators are RVs so the value may not match the integral, however provided the samples cover $\Omega$ it will be unbiased. e.g. $E[\langle I \rangle] = I$  
This estimator is also "consistent", as $M \to \infty$, $\langle I \rangle \to I$ 

A perfect PDF is one such that:  
$\forall x \in \Omega, \dfrac{f(x)}{p(x)} = 1$  
There is no variance  
However this PDF is impractical as it requires knowing the value of $I$ ahead of integration  
Our goal is to bring the effective PDF as close as possible to the perfect PDF 

$V[\langle I \rangle] = E[(\langle I \rangle - E[\langle I \rangle])^2]$  
Deviation of $E[\langle I \rangle]$ from $I$ is called bias

Function Support:  
$supp(f)$ is the set of all $x$ where $f(x) \neq 0$  
The inputs which have a non-zero output  
Can be negative  

RV Support:  
$supp(X)$ is the set of possible inputs for $X$  
A uniform RV $X$ from 0 to 1 has $supp(X) = [0,1]$ 
$supp(X)$ is the set of all x where $p_X(x) > 0$  

If $X$ has PDF $p_X$ then $supp(X) = supp(p_X)$ 

For an estimator to be unbiased, $supp(X)$ must contain $supp(f)$  
$supp(f) \subseteq supp(X)$  
$X$ "covers" $f$  

Traditionally this requirement rarely poses an issue and doesn't factor into algorithm design. But as ReSTIR mixes samples from many different distributions that do not necessarily cover the integrand, extra care needs to be taken to ensure unbiasedness 

### MIS 

A naive MC estimator has a problem. Not every RV is equally good at sampling $f$ everywhere, but by averaging samples you are adding their variances giving the worst of both worlds. You may also get bias if any RVs don't cover $f$  

MIS solves this by performing a weighted combination instead  

$\langle I \rangle = \sum m_i(X_i) \dfrac{f(X_i)}{p_i(X_i)}$  
$m_i(X_i)$ : MIS weight of the $i$th RV.  

In order for the MIS weights to be unbiased,  
$x \in supp(X_i) \to \sum_i m_i(x) = 1$  
$x \not \in supp(X_i) \to m_i(x) = 0$  

The naive MC estimator corresponds to $m_i = \frac{1}{M}$. Is only unbiased if all $X_i$ cover $f$  

A better choice is the balance heuristic:  
$m_i(x) = \dfrac{p_i(x)}{\sum_j p_j(x)}$  
This is optimal unless you consider negative weights which can do even better  

### Unbiased Contribution Weights

So far we assumed $p(x)$ can be evaluated, but sometimes that may not be possible  

We can still perform unbiased MC integration as long as we know a RV $W_X$ whose expected value given $X$ matches the reciprocal PDF  
$E[W_X | X] = \dfrac{1}{p(X)}$  

$\langle I \rangle = f(X) \cdot W_X$  
$E[f(X) \cdot W_X] = E[f(X) / p(X)] = I$  

There are many simple formulae allowing $W_X$ to be evaluated even when $p(X)$ cannot. Samples produced by RIS fall into this category  

## Resampled Importance Sampling (RIS) 

Given a sequence of inputs $(X_1, \dots, X_M)$  
Calculate weights $w_i$ for all inputs  
Choose an input proportionally to the weights such that the selected samples $p$ is closer to $f$ 

RIS aggregates many samples into a single better-distributed one

This one output is NOT better than all the inputs combined. But this single sample has an advantage in that processing a single sample is much cheaper than processing $M$ samples.  
This cost difference increases drastically if we chain RIS 

Assume a RIS resampling from $M_2$ samples that each aggregated $M_1$ samples from the prior frame. The output is an aggregation of $M_1 \times M_2$ samples, but costs less than $M_1 + M_2$ samples. Repeat this for thousands of frames and also grab samples from neighbours each frame and you can see insane efficiency improvements  

Candidates are continuous RVs and the output is a continuous RV. Hence despite the discrete selection, RIS is comparable to path-guiding. However it does not learn a distribution based on existing samples, but re-uses one at random. Filtering the distribution.  

The PDF of the sample produced by RIS is typically intractable and cannot be evaluated. At least as much computation as shading the pixel. We can avoid this using $W_X$.  

A single sample $X$ can have many different valid $W_X$ depending on which candidate samples were used to select it. It is not determinstically dependent on $X$. It's a RV.  

$w_i = \dfrac{1}{M} \dfrac{\hat p(X_i)}{p(X_i)}$  

$W_X = \dfrac{1}{\hat p(X)} \sum w_i$ 

$\hat p(x)$ : The target function

Older ReSTIR papers before GRIS had the $\frac{1}{M}$ term inside the $W_X$ equation, it was changed as it leads to simpler algorithms

The $\frac{1}{M}$ term is the resampling MIS weight and will be replaced by $m_i$ when we start shift mapping  

When the variance of $\sum w_i$ approaches zero, the output PDF approaches the target PDF $\bar p$   
$\bar p = \dfrac{\hat p}{\int \hat p}$   
$W_X$ also approaches $\dfrac{1}{\bar p(X)}$ 

### The Target Function ($\hat p$) 

$\hat p$ is an unnormalized function that is an attempt to be a good match for the integrand $f$ 

$\hat p$ defines the target PDF $\bar p$ 

Choosing $\hat p = f$ turns RIS into a zero-variance estimator, producing samples proportionally to $f$  
This is very computationally expensive, but is a good starting point. Avoid premature optimization  

Always returns a scalar, not whatever the candidate domain is. 
For example if the candidate domain is a color in $\mathbb{R}^3$ then $\hat p$ returns the luminance in $\mathbb{R}$ 

### RIS Algorithm

1. Take candidates $(X_1, \dots, X_M)$ in $\Omega$  
2. Evaluate $m_i(X_i)$ for all $X_i$
3. Evaluate $w_i = m_i(X_i) \hat p(X_i) W_{X_i}$ for all $X_i$ 
4. Choose $X$ proportionally to $w_i$ 
5. Evaluate $W_X = \dfrac{1}{\hat p(X)} \sum w_i$  

```cpp
// T can be anything, a color, a struct of data, a path vertex, a BSDF (wi,pdf,f)
struct Sample
{
    T X;
    float W_X;
};

uint RandomIndex(float w[M], float sumWeights)
{
    float r = Rand01() * sumWeights;

    for (int s = 0; s < M; s++)
    {
        if (w[s] <= 0)
            continue;

        r -= w[s];
        if (r <= 0)
            return s;
    }

    return NULL;
}

float MIS(T);
float Target(T);
float UnbiasedContribution(T);

Sample RIS(M)
{
    T X[M];
    float W_X[M];
    float w[M];

    for (int i = 0; i < M; i++)
    {
        X[i] = Generate();

        float m_i = MIS(X[i]);
        float target = Target(X[i]);
        W_X[i] = UnbiasedContribution(X[i]);

        w[i] = m_i * target * W_X[i];
    }

    float sumWeights = 0;
    for (int s = 0; s < M; s++)
        sumWeights += w[s];

    uint s = RandomIndex(w, sumWeights);
    if (s != NULL)
    {   
        T Y = X[s];
        float W_Y = sumWeights / Target(Y);
        return { Y, W_Y };
    }

    return { NULL, 0 };
}
```

$supp(Y)$ is the union of the input supports, with $x$ where $\hat p(x) = 0$ removed. To avoid biasing the estimate, we must be able to select samples across all of $supp(f)$ 

If all $w_i$ are zero and no sample can be chosen, return a NULL sample with $W_Y$ = 0.  
Do not replace the null sample by immediately drawing another as that causes bias.  
If passing a RV with null sample realization to RIS, include it in other samples' MIS weights as usual 

### MIS Weights

If the candidates have different PDFs, such as when they are re-used across pixels/time, $m_i$ needs to be more advanced than $\frac{1}{M}$  

If all inputs individually cover the support of $\hat p$ then $\frac{1}{M}$ is technically unbiased although could result in terrible outliers in areas hard for just one of the inputs. If any input has zero PDF anywhere in $supp(f)$ then $\frac{1}{M}$ leads to bias

If the input samples' PDFs are known then you can use the balance heuristic  
Remember that the MIS weight does not care about realizations, only the distributions  
Use the balance heuristic for a first attempt and then replace with a more advanced one once it works  

The balance heuristic becomes more expensive with large sample counts. The MIS weight for each of the $M$ samples requires evaluating $M$ PDFs giving $O(M^2)$  
Large sample counts may benefit from more advanced MIS weights such as pairwise MIS  

With correctly computed MIS weights the supports of each candidate no longer must all cover $supp(f)$. Now only their union has to  
In practice this can be tricky to guarantee without at least one "canonical sample", a sample with covers all of $supp(f)$  
All unbiased ReSTIR algorithms inject at least one new canonical sample per pixel, per frame

#### Unknown PDFs

As input samples are generated with RIS, they are distributed approximately proportionally to the target functions used in the resampling. Assume each input is associated with a target function $\hat p_i$   

This results in the following, generalized balance heuristic:  
$m_i(x) = \dfrac{\hat p_i (x)}{\sum \hat p_j (x)}$  

These MIS weights will allow us to resample across pixels/frames. But only within the same domain and without modification at reuse.  

### Simple BSDF Importance Sampling Example

Let the candidates $X_i$ be directions  
Let $\hat p$ be a cheaper proxy for the full BSDF $f$ with the same support, representing how valuable a given direction is for sampling.  
Let $m_i = \frac{1}{M}$  
Use RIS to select a candidate and use $W_X$ instead of $\dfrac{1}{p(X)}$ for the sampled direction 

### BSDF / NEE Importance Sampling Example

Let the candidates $X_i$ be directions, whether from a BSDF or toward a light point  

Draw $M_1$ candidates from a BSDF importance sampler with PDF $p_1$  
Draw $M_2$ candidates from a light sampler with PDF $p_2$  
The PDFs are in the same measure. Area PDFs can be converted to Solid Angle PDFs by multiplying by the geometry term and vice versa.   
Let $\hat p$ be a cheaper proxy for the full path contribution  

For the BSDF samples:  
Let $m_i(x) = \dfrac{p_1(x)}{M_1 p_1(x) + M_2 p_2(x)}$ 
Let $w_i = m_i(X_i) \hat p(X_i) W_{X_i}$  
Let $W_{X_i} = \dfrac{1}{p_1(X_i)}$ 

And similarly for the light samples but using $p_2$  

Next choose index $s$ proportionally to the $w_i$  
Let $W_X = \dfrac{1}{\hat p(X)} \overset{M_1 + M_2}{\sum} w_j$  

Technically using $m_i = \frac{1}{M_1 + M_2}$ would not cause bias since both PDFs cover all contributing direct illumination. However the noise level would be as if only sampling the BSDF - bad.  

## Spatiotemporal Reservoir Resampling

RIS is an effective way of improving the distribution of samples, however for complex $\hat p$ and poorly distributed candidates, the number $M$ of candidates required for good sampling may be too expensive  

ReSTIR addresses this by chaining invocations of RIS and reusing samples spatially and temporally.  
Reservoir Resampling is a practical improvement on RIS that will be useful to begin spatiotemporal reuse.  

### Weighted Reservoir Sampling (WRS)

In order to select the output sample, RIS needs to generate and store all candidates up-front before selecting the output sample in a second pass. This can be expensive, especially on GPUs.  

WRS is a family of algorithms for sampling 1+ elements from a weighted stream of samples in a single pass over the data without storing it  

WRS processes the elements in order, at each point possibly replacing the sample in the reservoir with the next sample in the stream

```cpp
struct Reservoir
{
    T Y;
    float W_Y;
    float WeightSum;
};

void Reservoir::Update(T X_i, float w_i)
{
    WeightSum += w_i;
    float r = Rand01();
    if (r < w_i/WeightSum)
        Y = X_i;
}

Reservoir Resample(M)
{
    Reservoir R;

    for (int i = 0; i < M; i++)
    {
        T X_i = Generate();
        float m_i = MIS(X_i);
        float target = Target(X_i);
        float W_X_i = UnbiasedContribution(X_i);

        float w_i = m_i * target * W_X_i;

        R.Update(X_i, w_i);
    }

    if (R.Y != NULL)
    {
        R.W_Y = R.WeightSum / Target(R.Y);
    }
    return R;
}
```

### Spatiotemporal Reuse

Start by producing an "initial candidate" sample approximately proportional to $\hat p$ by RIS from 1+ independent samples. If the inputs have identical distributions then you can use $m_i = \frac{1}{M}$ 

Then pick a set of spatial neighbours (e.g. picked randomly from a disk) and invoke RIS again, resampling from its own sample and the sample of selected neighbours  
This can be repeated multiple times at the cost of increased correlation  
$m_i$ is the generalized balance heuristic

Then use motion vectors to find the relevant pixel from the prior frame. If temporal reuse occurs each frame, samples feed forward through time indefinitely, continually improving the distribution.  
If temporal reuse is followed by spatial, samples from prior frames can spread spatially leading to rapid spread of good samples.  
Access to previous frames' $\hat p$ is required for MIS to remove bias

Finally compute and return $f(Y) \cdot W_Y$ 

## ReSTIR DI

Direct lighting is the contribution of all length-3 paths  
$\bar x = [x_0, x_1, x_2]$  
$x_0$ is on the image plane  
$x_1$ is the primary hit, reflecting off a surface/particle  
$x_2$ lies on an emissive light surface  

Let $A$ be the set of points on emissive surfaces  
$x_2 \in A$  

$x_0, x_1$ may be deterministic or depend on randomized lens coordinates (DoF). Post-randomization, we treat their values as fixed, making $x_2$ the only free variable.  
Our paths are such only functions of $x_2$  

We want to integrate $x_2$ over the surface geometry, and improve its distribution using ReSTIR.  

$L(x_1 \to x_0) = \int_A f_s(x_2 \to x_1 \to x_0) G(x_2 \leftrightarrow x_1) V(x_2 \leftrightarrow x_1) L_e (x_2 \to x_1) \ dx_2$   
$f_s$ : BSDF at $x_1$  
$L_e$ : Emission from $x_2$  
$G$   : Geometry term
$V$   : Visibility term

Treating $x_0$ and $x_1$ as constants:  
$L(x_1 \to x_0) = \int_A f(x_2) \ dx_2$  

As $x_0$ and $x_1$ vary by pixel index $i$, we have pixel-dependent $f_i$ over the same space $A$.  

We want to share $x_2$ between pixels, to do so we resample $x_2$ from 1+ independent canonical samples covering the current pixel, plus samples from other pixels/frames.  

For simplicity of notation, let $x \equiv x_2$ from here and let $x_0, x_1$ be implicit constants  
$f(x) = f_s(x) G(x) V(x) L_e(x)$  

A possible good optimization is dropping $V$ from $\hat p$. But it worses the theoretical limit distribution and requires additional conditions to remain correct

Start by generating canonical samples for each pixel with RIS from multiple canonical inputs. For basic DI we may pick $M$ samples on emitting surfaces using a standard light sampler and pick one using RIS with $m_i = \frac{1}{M}$  

Then perform spatiotemporal reuse.  

Use $\hat p_j$ based on pixel $j$ and $x_{j,0}, x_{j,1}$  
$m_i(x) = \dfrac{\hat p_i(x)}{\sum \hat p_j (x)}$  

For spatial reuse you should pick a suitable number of pixels from the relative vicinity of the pixel, e.g. a square or disk.  
Using GBuffer values to heuristically choose pixels should be fine as long as decisions are not based on samples stored in the reservoirs  

It's recommended to implement and validate with only candidate samples first, then add spatial, then temporal without motion, then full temporal.   
Validate by ensuring a large number of still frames converge to the ground truth.  

If the scene geometry is dynamic, you need to store the prior frames acceleration structure to properly compute $\hat p$ from the prior frame. 

### Confidence Weights

The above approach works but is inefficient. It weights the prior frame and the current frame with the same weight, this causes it to lose ~50% of the accumulated history each frame. This can be fixed by weighted MIS, introducing confidence weights  

We give each sample confidence weights $c_j$ stored in the pixel reservoirs.  
These are then used for computing the MIS weights:  
$m_i(x) = \dfrac{c_i \hat p_i(x)}{\sum c_j \hat p_j(x)}$  

The more trustworthy an RV is, the higher its confidence $c_i$ should be.  
If one input corresponds to 7 independent samples of a kind, while another corresponds to 2, the confidences should be 7 and 2.  

When aggregating samples of confidence $c_1$ and $c_2$ with RIS, we set the confidence to $c_1 + c_2$.  
This is an upper bound of the effective sample count, but a more accurate estimate is hard to get.  
Over multiple frames these confidences would grow exponentially, a drastic overestimate causing convergence to a wrong result. This is prevented using M-Capping. The confidence is commonly capped to around $c_{cap} = 5-30$. This constant defines the balance between noise and correlation 

#### Resampling with Confidence Weights Algorithm

```cpp
struct Reservoir
{
    T Y;
    float W_Y;
    float WeightSum;
    float Confidence;
};

void Reservoir::Update(T X_i, float w_i, float c_i)
{
    WeightSum += w_i;
    Confidence += c_i;

    float r = Rand01();
    if (r < w_i/WeightSum)
        Y = X_i;
}

Reservoir Resample(M)
{
    Reservoir R;

    for (int i = 0; i < M; i++)
    {
        T X_i = Generate();
        float m_i = MIS(X_i);
        float target = Target(X_i);
        float W_X_i = UnbiasedContribution(X_i);
        float c_i = 1;

        float w_i = m_i * target * W_X_i;

        R.Update(X_i, w_i, c_i);
    }

    if (R.Y != NULL)
    {
        R.W_Y = R.WeightSum / Target(R.Y);
    }
    R.Confidence = min(R.Confidence, c_cap);
    return R;
}
```

Pixels entering the screen as the camera moves get their confidence reset to 0. It also often makes sense to do so when detecting occlusions/disocclusions.  
Resetting confidence can only be done if it's solely based on the GBuffer

### Improved Sampling

Just using a light sampler for ReSTIR DI is not ideal, a BSDF sampler would benefit glossy materials.  

It's recommended not to implement BSDF sampling until light sampling ReSTIR DI works.  

Improving Light Sampling:  
Candidate samples can be cheaply generated with power-based importance sampling, a light source is stochastically selected based on its total emitted flux over the surface. A sample point is picked uniformly from its surface area.   
Can be good to generate 32 candidate light samples and pick one with WRS. Can be further optimized by precomputing the light samples into "light tiles" shared by screen pixel blocks

## Shift Mapping

So far ReSTIR reuses samples within the same domain. But if the objects move and the domain changes between frames then samples need to be modified to enable reuse between frames. This requires shift mappings and an extension of RIS.  

Reusing vertices without modification does not allow reuse through mirrors / glass. The law of ideal reflection must be obeyed.  

An example of a shift mapping would be gluing the path vertices to the moving objects, matching the triangle index and UV in both frames

In light transport, shift mappings allow reusing paths between domains such as path spaces seen by different pixels.  

A shift mapping $T$ from $A$ to $B$ (path spaces of different pixels) maps paths in $A$ to paths in $B$ by a relation $y = T(x)$ 

Shift mappings must be determinisitc and bijective, an inverse shift must exist.    
$T_{i\to j} \circ T_{j\to i} (x) = x$  
Not all paths need to be shiftable.   
If $x$ cannot be shifted with $T_{i\to j}$ then no path $y$ is allowed to shift to $x$  

In practice, invertibility is often guaranteed by symmetric shift mappings.  
If during the shifting process, the code finds a show-stopper condition such as an occlusion in reconnection shift, it forces $T_{i\to j}$ to halt and return undefined.  

As shift mapping map paths between domains, they also modify the path densities. Jacobian determinants $|T'(x)|$ capture the local scaling factor. This changes the PDFs and unbiased contributions when passing RVs through mappings.  
If $Y = T(X)$:  
$p_Y(Y) = \dfrac{p_X(X)}{|T'(X)|}$  
$W_Y = W_X |T'(x)|$  

### Reconnection Shift

Maps a path to another pixel, reconnecting the deterministic beginning to the same $x_2$, retaining all free vertices.  

$T_{i\to j}([x_{i,0}, x_{i,1}, x_2, x_3, \dots]) = [x_{j,0}, x_{j,1}, x_2, x_3, \dots]$ 

Always implement this shift mapping first. Validate that shifting to the same pixel retains the path and its radiance, and that the jacobian determinent is 1:  
$T_{i\to i}(x) = x$   
$f(T_{i\to i}(x)) = f(x)$  
$|T_{i\to i}'(x)| = 1$  

This mapping works well for diffuse/rough surfaces but not for specular/glossy surfaces

### Half-Vector Shift

Instead of copying the outgoing direction $x_{j,1} \to x_2$, copy the half-vector H

Shoot a new primary ray $x_{j,0} \to x_{j,1}$  
Transform the original H into the tangent space of $x_{j,1}$  
Reconstruct the reflected direction satisfying:  
$w_o = 2(w_i \cdot h) h - w_i$  
Trace this new reflected ray  

The BSDF lobe stays aligned, mirror/glossy reflections remain very similar. Nearby pixels produce corresponding reflected paths  

It is not guaranteed that the shifted paths hits $x_2$. It only tries to produce a corresponding path whose probability can be related to the original path

### Random-Replay Shift

Instead of copying geometry, it copies the RNG that originally generated the path  
Includes the RNG for every BSDF sample, light sample, RR, etc  
I assume all you need to do is copy over the old seed  

The advantage is that it's super cheap and easy for GPUs. The drawback is that longer paths may diverge significantly   

### Hybrid Shift

Uses Random-Replay for the initial specular/glossy portions of the path. Then uses reconnection once both paths reach sufficiently rough surfaces

### RIS with Shift Mapping Algorithm

Take inputs $(X_1, \dots, X_M)$ from domains $\Omega_i$  
Map samples into the target domain $\Omega$ with $Y_i = T_i(X_i)$   
Evaluate all $m_i(Y_i)$   
Evaluate all $W_{Y_i} = W_{X_i} |T_i'(X_i)|$   
Evaluate all $w_i = m_i(Y_i) \hat p(Y_i) W_{Y_i}$  
Choose $Y$ proportionally to $w_i$  
Evaluate $W_Y$  

If a shift mapping fails then $w_i = 0$  

Continue from pg 32. MIS between domains  