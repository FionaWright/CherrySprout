#include "Utils/SharedUtils.h"
#include "Utils/CBVs.h"

#include "Compute/PoissonSolvers/JacobiBuffers.hlsli"

float3 SampleSafeZero(Texture2D tex, int2 pixelCoord, uint width, uint height)
{
    if (pixelCoord.x < 0 || pixelCoord.y < 0)
        return 0;

    if (pixelCoord.x == width || pixelCoord.y == height)
        return 0;

    return tex[pixelCoord].rgb;
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

    float3 primal = gTexPrimal[pixelCoord].rgb;

    float3 b = gCBV.Alpha * primal - BackwardsDivergence(pixelCoord);

    gGradientTerms[pixelCoord].rgb = b;
}