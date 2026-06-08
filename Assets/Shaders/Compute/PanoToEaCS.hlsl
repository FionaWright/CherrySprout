#include "Utils/SharedUtils.h"
#include "Utils/CBVs.h"

Texture2D<float3> gPano : register(t0);
RWTexture2D<float4> gEA : register(u0);

SamplerState gSampler : register(s0);

ConstantBuffer<CbvPanoToEA> cbv : register(b0);

[numthreads(16,16,1)]
void CSMain(uint3 DTid : SV_DispatchThreadID)
{
    if (DTid.x >= cbv.OutputDimensions.x || DTid.y >= cbv.OutputDimensions.y)
        return;

    // Normalized coordinates in [0,1]
    float u = (DTid.x + 0.5f) / cbv.OutputDimensions.x;
    float v = (DTid.y + 0.5f) / cbv.OutputDimensions.y;

    float3 dir = EaSquareToSphere(float2(u, v));
    float2 panoUV = PanoSphereToSquare(dir);
    panoUV.x = frac(panoUV.x + cbv.Rotation);
    panoUV.y = 1 - panoUV.y;

    float3 color = gPano.SampleLevel(gSampler, panoUV, 0.0f).rgb;

    gEA[DTid.xy] = float4(color, 1);
}
