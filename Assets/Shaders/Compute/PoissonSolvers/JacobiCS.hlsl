#include "Utils/SharedUtils.h"
#include "Utils/CBVs.h"

#include "Compute/PoissonSolvers/JacobiBuffers.hlsli"

float3 SampleSafeClamp(Texture2D tex, int2 pixelCoord, uint width, uint height)
{
    pixelCoord.x = min(width-1, max(0, pixelCoord.x));
    pixelCoord.y = min(height-1, max(0, pixelCoord.y));

    return tex[pixelCoord].rgb;
}

float3 JacobiUpdateX(int2 pixelCoord, float3 b)
{
    float c = gCBV.JacobiCoefficient;
    float a = gCBV.Alpha;

    float3 cAminusI = (c * a - c + (c * 4.0f)) * gPreviousIteration[pixelCoord].rgb;

    uint w = gCBV.PrimalWidth;
    uint h = gCBV.PrimalHeight;

    float3 neighbours =
        SampleSafeClamp(gPreviousIteration, pixelCoord + int2(-1,0), w, h) +
        SampleSafeClamp(gPreviousIteration, pixelCoord + int2(0,-1), w, h) +
        SampleSafeClamp(gPreviousIteration, pixelCoord + int2(1, 0), w, h) +
        SampleSafeClamp(gPreviousIteration, pixelCoord + int2(0, 1), w, h);

    cAminusI -= c * neighbours;

    return b * c - cAminusI;
}

float3 JacobiUpdate(int2 pixelCoord, float3 b)
{
    float c = gCBV.JacobiCoefficient;

    uint w = gCBV.PrimalWidth;
    uint h = gCBV.PrimalHeight;

    float3 neighbours =
        SampleSafeClamp(gPreviousIteration, pixelCoord + int2(-1,0), w, h) +
        SampleSafeClamp(gPreviousIteration, pixelCoord + int2(0,-1), w, h) +
        SampleSafeClamp(gPreviousIteration, pixelCoord + int2(1, 0), w, h) +
        SampleSafeClamp(gPreviousIteration, pixelCoord + int2(0, 1), w, h);

    return (b + c * neighbours) / (gCBV.Alpha + 4.0f * c);
}

[numthreads(16,16,1)]
void CSMain(uint3 DTid : SV_DispatchThreadID)
{
    uint2 pixelCoord = DTid.xy;

    if (pixelCoord.x >= gCBV.PrimalWidth || pixelCoord.y >= gCBV.PrimalHeight)
        return;

    float3 b = gGradientTerms[pixelCoord].rgb;
    gNextIteration[pixelCoord].rgb = JacobiUpdateX(pixelCoord, b);
}
