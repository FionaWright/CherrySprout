#include "Utils/SharedUtils.h"
#include "Utils/CBVs.h"

#include "Compute/PoissonSolvers/JacobiBuffers.hlsli"

float3 SampleSafeClamp(Texture2D tex, int2 pixelCoord, uint width, uint height)
{
    pixelCoord.x = min(width, max(0, pixelCoord.x));
    pixelCoord.y = min(height, max(0, pixelCoord.y));

    return tex[pixelCoord].rgb;
}

// default c = 0.25f
float3 ComputeAMinusI(int2 pixelCoord, float3 b)
{
    float3 v = (gCBV.Alpha - 1 + (gCBV.JacobiCoefficient * 4.0f)) * gPreviousIteration[pixelCoord].rgb;

    uint w = gCBV.PrimalWidth;
    uint h = gCBV.PrimalWidth;

    v -= gCBV.JacobiCoefficient * SampleSafeClamp(gPreviousIteration, pixelCoord + int2(-1,0), w, h);
    v -= gCBV.JacobiCoefficient * SampleSafeClamp(gPreviousIteration, pixelCoord + int2(0,-1), w, h);
    v -= gCBV.JacobiCoefficient * SampleSafeClamp(gPreviousIteration, pixelCoord + int2(1, 0), w, h);
    v -= gCBV.JacobiCoefficient * SampleSafeClamp(gPreviousIteration, pixelCoord + int2(0, 1), w, h);

    return v;
}

[numthreads(16,16,1)]
void CSMain(uint3 DTid : SV_DispatchThreadID)
{
    uint2 pixelCoord = DTid.xy;

    if (pixelCoord.x >= gCBV.PrimalWidth || pixelCoord.y >= gCBV.PrimalHeight)
        return;

    float3 b = gGradientTerms[pixelCoord].rgb;

    float3 aMinusI = ComputeAMinusI(pixelCoord, b);
    gNextIteration[pixelCoord].rgb = aMinusI;
}
