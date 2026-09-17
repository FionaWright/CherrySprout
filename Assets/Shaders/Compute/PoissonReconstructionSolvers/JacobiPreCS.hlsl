#include "Utils/SharedUtils.h"
#include "Utils/CBVs.h"

Texture2D<float4> gTexPrimal : register(t0);
Texture2D<float4> gTexGradientX : register(t1);
Texture2D<float4> gTexGradientY : register(t2);

RWTexture2D<float4> gGradientTerms : register(u0); // b_{i,j}

ConstantBuffer<CbvSprJacobi> gCBV : register(b0);

float3 SampleSafeZero(Texture2D tex, int2 pixelCoord, uint width, uint height)
{
    if (pixelCoord.x < 0 || pixelCoord.y < 0)
        return 0;

    if (pixelCoord.x == width || pixelCoord.y == height)
        return 0;

    return tex[pixelCoord];
}

float3 BackwardsDivergence(int2 pixelCoord)
{
    float3 v = 0.0f;

    uint gradientXWidth = gCBV.PrimalWidth - 1;
    uint gradientYWidth = gCBV.PrimalWidth;
    uint gradientXHeight = gCBV.PrimalHeight;
    uint gradientYHeight = gCBV.PrimalHeight - 1;

    v += SampleSafeZero(gTexGradientX, pixelCoord, gradientXWidth, gradientXHeight) - SampleSafeZero(gTexGradientX, pixelCoord + int2(-1,0), gradientXWidth, gradientXHeight);
    v += SampleSafeZero(gTexGradientY, pixelCoord, gradientYWidth, gradientYHeight) - SampleSafeZero(gTexGradientY, pixelCoord + int2(0,-1), gradientYWidth, gradientYHeight);

    return v;
}

[numthreads(16,16,1)]
void CSMain(uint3 DTid : SV_DispatchThreadID)
{
    uint2 pixelCoord = DTid.xy;

    if (pixelCoord.x >= gCBV.PrimalWidth || pixelCoord.y >= gCBV.PrimalHeight)
        return;

    float3 primal = gTexPrimal[pixelCoord];

    float3 b = gCBV.Alpha * primal - BackwardsDivergence(pixelCoord);

    gGradientTerms[pixelCoord] = b;
}