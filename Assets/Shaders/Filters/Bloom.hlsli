#include "Utils/CBVs.h"
#include "Utils/SharedUtils.h"

ConstantBuffer<CbvFilterBloom> gCBV : register(b0);

Texture2D<float4> gSource : register(t0);
RWTexture2D<float4> gDest : register(u0);

SamplerState gSamplerLinear : register(s0);

#define MAX_RADIUS 7

float Gaussian(float x, float sigma)
{
    return exp(-(x*x) / (2.0 * sigma * sigma));
}

float3 ExtractBright(uint2 pixelCoord)
{
    float3 v = gSource[pixelCoord].rgb;

    float lum = Luminance(v);

    if (lum < gCBV.LuminanceThreshold)
        return 0;

    return v;
}

float3 Downsample(uint2 pixelCoord)
{
    float2 uv = (float2(pixelCoord) + 0.5f) / float2(gCBV.DestDimensions);

    float3 v = gSource.Sample(gSamplerLinear, uv).rgb;
    return v;
}

float3 Upsample(uint2 pixelCoord)
{
    float2 uv = pixelCoord / float2(gCBV.DestDimensions);

    float3 vS = gSource.Sample(gSamplerLinear, uv).rgb;
    float3 vD = gDest[pixelCoord].rgb;
    return vS + vD;
}

float3 Blur(uint2 pixelCoord)
{
    uint2 dim;
    gSource.GetDimensions(dim.x, dim.y);

    float weights[MAX_RADIUS + 1];

    float sigma = gCBV.Radius / 3.0f;
    float weightSum = 0.0f;

    for (int i = 0; i <= gCBV.Radius; i++)
    {
        weights[i] = Gaussian(i, sigma);
        weightSum += (i == 0) ? weights[i] : weights[i] * 2.0f; // *2 due to +x and -x
    }

    float3 sum = 0.0;

    for (int x = -gCBV.Radius; x <= gCBV.Radius; ++x)
    {
#ifdef HORIZONTAL
        int2 offset = int2(x, 0);
#else
        int2 offset = int2(0, x);
#endif
        int2 coord = int2(pixelCoord) + offset;

        if (any(coord < 0) || any(coord >= dim))
            continue;

        sum += gSource[coord].rgb * weights[abs(x)];
    }

    return sum / weightSum;
}