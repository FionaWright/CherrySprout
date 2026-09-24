#include "Utils/SharedUtils.h"
#include "Utils/CBVs.h"

#include "Compute/PoissonSolvers/JacobiBuffers.hlsli"

float3 SampleSafeZero(Texture2D tex, int2 pixelCoord, uint min, uint width, uint height)
{
    if (pixelCoord.x < min || pixelCoord.y < min)
        return 0;

    if (pixelCoord.x >= width || pixelCoord.y >= height)
        return 0;

    return tex[pixelCoord].rgb;
}

float3 GetTrueGradientX(int2 pixelCoord)
{
    uint w1 = gCBV.PrimalWidth - 1;
    uint w = gCBV.PrimalWidth;
    uint h = gCBV.PrimalHeight;

    float3 gradientXF = SampleSafeZero(gTexGradientXF, pixelCoord,              0,  w1, h);
    float3 gradientXB = SampleSafeZero(gTexGradientXB, pixelCoord + uint2(1,0), 1,  w,  h);

    return gradientXF + gradientXB;
}

float3 GetTrueGradientY(int2 pixelCoord)
{
    uint h1 = gCBV.PrimalHeight - 1;
    uint w = gCBV.PrimalWidth;
    uint h = gCBV.PrimalHeight;

    float3 gradientYF = SampleSafeZero(gTexGradientYF, pixelCoord,              0,  w,  h1);
    float3 gradientYB = SampleSafeZero(gTexGradientYB, pixelCoord + uint2(0,1), 1,  w,  h);

    return gradientYF + gradientYB;
}

// Note: Needs to be opposite of { forward, backward } difference that gradients were computed by such that the computation is for the central difference
float3 BackwardsDivergence(int2 pixelCoord)
{
    float3 v = 0.0f;
    v += GetTrueGradientX(pixelCoord) - GetTrueGradientX(pixelCoord + int2(-1,0));
    v += GetTrueGradientY(pixelCoord) - GetTrueGradientY(pixelCoord + int2(0,-1));
    return v;
}

[numthreads(16,16,1)]
void CSMain(uint3 DTid : SV_DispatchThreadID)
{
    uint2 pixelCoord = DTid.xy;

    if (pixelCoord.x >= gCBV.PrimalWidth || pixelCoord.y >= gCBV.PrimalHeight)
        return;

    float3 primal = gTexPrimal[pixelCoord].rgb;

    float3 b = -gCBV.Alpha * primal + BackwardsDivergence(pixelCoord);

    gGradientTerms[pixelCoord].rgb = b;
}