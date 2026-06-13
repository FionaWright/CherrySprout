#include "Utils/SharedUtils.h"
#include "Utils/CBVs.h"

Texture2D<float4> gIn : register(t0);
RWTexture2D<float4> gOut : register(u0);

SamplerState gSampler : register(s0);

ConstantBuffer<CbvGammaCorrect> cbv : register(b0);

[numthreads(16,16,1)]
void CSMain(uint3 DTid : SV_DispatchThreadID)
{
    if (DTid.x >= cbv.Dimensions.x || DTid.y >= cbv.Dimensions.y)
        return;

    float2 uv = (DTid.xy + 0.5f) / cbv.Dimensions.xy;
    float3 color = gIn.SampleLevel(gSampler, uv, 0.0f).rgb;

    if (cbv.IsToSrgb)
        color = pow(color, 1.0f / 2.2f);
    else
        color = pow(color, 2.2f);

    gOut[DTid.xy] = float4(color, 1);
}
