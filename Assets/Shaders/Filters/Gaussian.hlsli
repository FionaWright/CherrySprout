#include "Utils/CBVs.h"

ConstantBuffer<CbvFilterBoxAndGauss> gCBV : register(b0);

Texture2D<float4> gSource : register(t0);
RWTexture2D<float4> gDest : register(u0);

#define MAX_RADIUS 7

float Gaussian(float x, float sigma)
{
    return exp(-(x*x) / (2.0 * sigma * sigma));
}

[numthreads(16, 16, 1)]
void CSMain(uint3 DTid : SV_DispatchThreadID)
{
    uint2 pixelCoord = DTid.xy;

    float weights[MAX_RADIUS + 1];

    float sigma = gCBV.Radius / 3.0f;
    float weightSum = 0.0f;

    for (int i = 0; i <= gCBV.Radius; i++)
    {
        weights[i] = Gaussian(i, sigma);
        weightSum += (i == 0) ? weights[i] : weights[i] * 2.0f; // *2 due to +x and -x
    }

    uint2 dim;
    gSource.GetDimensions(dim.x, dim.y);

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

    gDest[pixelCoord].rgb = sum / weightSum;
}