#include "Utils/SharedUtils.h"
#include "Utils/CBVs.h"

Texture2D<float> gPMF : register(t0);
RWTexture2D<float> gCDF  : register(u0);

// Could be sped up by using parallel prefix scan (Blelloch/Hilis-Steele), but probably better to just cache the CDF if it's too slow

[numthreads(64,1,1)]
void CSMain(uint3 DTid : SV_DispatchThreadID)
{
    uint2 dim;
    gPMF.GetDimensions(dim.x, dim.y);

    uint y = DTid.x;

    if (y >= dim.y)
        return;

    float rollingSum = 0.0f;

    for (int x = 0; x < dim.x; x++)
    {
        rollingSum += gPMF.Load(int3(x, y, 0));
        gCDF[int2(x,y)] = rollingSum;
    }
}
