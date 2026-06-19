#include "Utils/SharedUtils.h"
#include "Utils/CBVs.h"

RWTexture2D<float> gCDF  : register(u0);

[numthreads(64,1,1)]
void CSMain(uint3 DTid : SV_DispatchThreadID)
{
    uint2 dim;
    gCDF.GetDimensions(dim.x, dim.y);

    uint y = DTid.x;

    if (y >= dim.y)
        return;

    float rowCdfLast = gCDF[uint2(dim.x - 1, y)];

    for (int x = 0; x < dim.x; x++)
    {
        gCDF[int2(x,y)] /= rowCdfLast;
    }
}
