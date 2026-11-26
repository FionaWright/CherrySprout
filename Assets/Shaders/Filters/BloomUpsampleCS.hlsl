#define HORIZONTAL

#include "Utils/CBVs.h"
#include "Filters/Bloom.hlsli"

[numthreads(16, 16, 1)]
void CSMain(uint3 DTid : SV_DispatchThreadID)
{
    uint2 pixelCoord = DTid.xy;

    uint2 dim;
    gSource.GetDimensions(dim.x, dim.y);

    if (any(pixelCoord > dim))
        return;

    gDest[pixelCoord].rgb = Upsample(pixelCoord);
}