# RNG

## Core

JitterPerPixel: x2 (01,01)
DepthOfField: x2 (01,01)

## Per-Ray

If Miss/Emissive: break

AlphaTesting: x1 (01)
NEE LSD: x1 (01Ex)

NEE EnvMap: x2 (01,01)
or
NEE Punc: x0
or
NEE Area: x3 (01,01,URange)

Microfacet: x2* (01,01,01?)
BxDF Transmission: x1 (01)
BxDF Specular/Reflect: x1 (01)

If Beta Loss: break

RR: x1 (01)

# Idea

x4 Core  
x0-10 Per-Ray  

```cpp
struct RngValuesPrimary
{
    float2 Jitter;
    float2 DoF;
};

struct RngValuesAncillary
{
    // Batch 1
    float AlphaTest;

    struct RVANEE
    {
        float LSD;
        float2 Light;
    } NEE[MAX_DIRECT_SAMPLES];

    float2 Microfacet;
    float Transmission;
    float Specular;

    // Batch 2
    float RussianRoulette;
};
```

