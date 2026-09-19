#include "Utils/SharedUtils.h"
#include "Utils/CBVs.h"

#include "Compute/PoissonSolvers/JacobiBuffers.hlsli"

// See Notes/Research/JacobiSPR.md

float3 SampleSafeZero(Texture2D tex, int2 pixelCoord, uint width, uint height, inout float a_coef)
{
    bool invalidPixel = false;
    if (pixelCoord.x < 0 || pixelCoord.x == width)
    {
        a_coef += 1.0f;
        invalidPixel = true;
    }

    if (pixelCoord.y < 0 || pixelCoord.y == height)
    {
        a_coef += 1.0f;
        invalidPixel = true;
    }

    if (invalidPixel)
        return 0;

    return tex[pixelCoord].rgb;
}

float3 JacobiUpdate(int2 pixelCoord, float3 b)
{
    float a = gCBV.Alpha;

    uint w = gCBV.PrimalWidth;
    uint h = gCBV.PrimalHeight;

    float a_coef = 0.0;
    float3 neighbours =
        SampleSafeZero(gPreviousIteration, pixelCoord + int2(-1,0), w, h, a_coef) +
        SampleSafeZero(gPreviousIteration, pixelCoord + int2(0,-1), w, h, a_coef) +
        SampleSafeZero(gPreviousIteration, pixelCoord + int2(1, 0), w, h, a_coef) +
        SampleSafeZero(gPreviousIteration, pixelCoord + int2(0, 1), w, h, a_coef);

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
