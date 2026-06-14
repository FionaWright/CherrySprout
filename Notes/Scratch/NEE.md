# NEE 

## Environment Map

Need:
`Texture2D<float> gEnvMapCdfConditional`
`Texture1D<float> gEnvMapCdfMarginal`

`gEnvMapPdfConditional[x,y] = Luminance(gEnvMap[x,y])`
`W = Sum^Width (gEnvMapPdfConditional[x_i])`
`gEnvMapPdfConditional[x] /= W`

`gEnvMapCdfConditional[x,y] = Sum^x (gEnvMapPdfConditional[x_i, y])` 

`gEnvMapPdfMarginal[y] = gEnvMapCdfConditional[Width-1, y]`
`gEnvMapCdfMarginal[y] = Sum^y (gEnvMapPdfMarginal[y_i])`

`N = Width * Height` 

Maths only correct for EA-maps. Remove pano maps from feature list

```cpp
float PowerHeuristic(float pdf1, float pdf2)
{
    pdf1_2 = pdf1 * pdf1;
    pdf2_2 = pdf2 * pdf2;
    return pdf1_2 / (pdf1_2 + pdf2_2);
}
```

### Sampling (Simple Binary Search)

Generate two random numbers u1, u2 in [0,1]

```cpp

L_sample = 0;

L_sample += beta * Emission;

// SampleEnvMap(float u1, float u2, out float3 wi_env, out float pdf_env);
{
    y = BinarySearch(gEnvMapCdfMarginal, u1);
    x = BinarySearch(gEnvMapCdfConditional[y], u2);
    wi_env = EaSquareToSphere(x, y);
    pdf_env = gEnvMapPdfConditional[x, y] * 4 * PI / N;
}

occuluded = ShadowRay(hitPos, wi_env);
if (!occluded)
{
    L_indirect = SampleEnvMap(x, y);

    f_env_bxdf = bxdf.Evaluate(wo, wi_env, pdf_env_bxdf);

    weight = PowerHeuristic(pdf_env, pdf_env_bxdf);
    
    float NdL = dot(N, wi_env);
    L_sample += beta * L_indirect * weight * NdL * f_env_bxdf / pdf_env;
}

bxdf.Sample(wo, wi, f_bxdf, pdf_bxdf);
beta *= f_bxdf;
```

### Sampling (Alias Table)