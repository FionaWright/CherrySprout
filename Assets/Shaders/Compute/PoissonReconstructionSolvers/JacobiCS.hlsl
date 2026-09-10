#include "Utils/SharedUtils.h"
#include "Utils/CBVs.h"

Texture2D<float4> gPreviousIteration : register(t0);
Texture2D<float4> gGradientTerms : register(t1); // b_{i,j}
RWTexture2D<float4> gNextIteration : register(u0);

[numthreads(16,16,1)]
void CSMain(uint3 DTid : SV_DispatchThreadID)
{
    if (DTid.x >= cbv.OutputDimensions.x || DTid.y >= cbv.OutputDimensions.y)
        return;

    left = gPreviousIteration[x-1,y];
    right = gPreviousIteration[x+1,y];
    up = gPreviousIteration[x,y-1];
    down = gPreviousIteration[x,y+1];

    b = gGradientTerms[x,y];

    gNextIteration[x,y] = (left + down + up + right + b) / (4 + lambda);
}
