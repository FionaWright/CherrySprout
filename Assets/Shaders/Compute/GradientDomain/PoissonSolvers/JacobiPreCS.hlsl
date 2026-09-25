#include "Utils/SharedUtils.h"
#include "Utils/CBVs.h"

#include "Compute/GradientDomain/PoissonSolvers/JacobiBuffers.hlsli"

#include "Compute/GradientDomain/Utils.hlsli"

// Note: Needs to be opposite of { forward, backward } difference that gradients were computed by such that the computation is for the central difference
float3 BackwardsDivergence(int2 pixelCoord)
{
    float3 v = 0.0f;

    v += GetTrueGradientX(gTexGradientXF, gTexGradientXB, pixelCoord, gCBV.PrimalWidth, gCBV.PrimalHeight);
    v -= GetTrueGradientX(gTexGradientXF, gTexGradientXB, pixelCoord + int2(-1,0), gCBV.PrimalWidth, gCBV.PrimalHeight);

    v += GetTrueGradientY(gTexGradientYF, gTexGradientYB, pixelCoord, gCBV.PrimalWidth, gCBV.PrimalHeight);
    v -= GetTrueGradientY(gTexGradientYF, gTexGradientYB, pixelCoord + int2(0,-1), gCBV.PrimalWidth, gCBV.PrimalHeight);

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