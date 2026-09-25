#include "Utils/SharedUtils.h"
#include "Utils/CBVs.h"

#include "Compute/GradientDomain/PoissonSolvers/JacobiBuffers.hlsli"

#include "Compute/GradientDomain/Utils.hlsli"

// See Notes/Research/JacobiSPR.md

float3 JacobiUpdate(int2 pixelCoord, float3 b)
{
    float a = gCBV.Alpha;

    uint w = gCBV.PrimalWidth;
    uint h = gCBV.PrimalHeight;

    float a_coef = 0.0;
    float3 neighbours =
        SampleSafeZeroCoef(gPreviousIteration, pixelCoord + int2(-1,0), w, h, a_coef) +
        SampleSafeZeroCoef(gPreviousIteration, pixelCoord + int2(0,-1), w, h, a_coef) +
        SampleSafeZeroCoef(gPreviousIteration, pixelCoord + int2(1, 0), w, h, a_coef) +
        SampleSafeZeroCoef(gPreviousIteration, pixelCoord + int2(0, 1), w, h, a_coef);

    float diag = a + 4.0f - a_coef;

    float3 LUx = -neighbours;

    return -(LUx + b) / diag;
}

[numthreads(16,16,1)]
void CSMain(uint3 DTid : SV_DispatchThreadID)
{
    uint2 pixelCoord = DTid.xy;

    if (pixelCoord.x >= gCBV.PrimalWidth || pixelCoord.y >= gCBV.PrimalHeight)
        return;

    float3 b = gGradientTerms[pixelCoord].rgb;
    gNextIteration[pixelCoord].rgb = JacobiUpdate(pixelCoord, b);
}
