#include "Utils/SharedUtils.h"
#include "Utils/CBVs.h"

Texture2D<float4> gEnvMap : register(t0);
RWTexture2D<float> gPMF  : register(u0);

ConstantBuffer<float> gSumLuminance : register(b0);

[numthreads(16,16,1)]
void CSMain(uint3 DTid : SV_DispatchThreadID)
{
    uint2 dim;
    gEnvMap.GetDimensions(dim.x, dim.y);

    if (DTid.x >= dim.x || DTid.y >= dim.y)
        return;

    float3 color = gEnvMap.Load(int3(DTid.xy, 0)).rgb;

    gOut[DTid.xy] = Luminance(color) / gSumLuminance;
}
