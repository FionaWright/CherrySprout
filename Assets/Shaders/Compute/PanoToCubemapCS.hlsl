#include "Utils/MathUtils.h"
#include "Utils/CBVs.h"

Texture2D<float3> gPano : register(t0);
RWTexture2DArray<float4> gCubemap : register(u0);

SamplerState gSampler : register(s0);

ConstantBuffer<CbvPanoToCM> cbv : register(b0);

[numthreads(16,16,1)]
void CSMain(uint3 DTid : SV_DispatchThreadID)
{
    if (DTid.x >= cbv.OutputWidth || DTid.y >= cbv.OutputWidth)
        return;

    uint face = DTid.z;
    float2 uv = (float2(DTid.xy) / float2(cbv.OutputWidth.xx - 1));
    uv = uv * 2.0f - 1.0f; // [-1, 1]

    float3 dir = CubemapCubeToSphere(face, uv);
    float2 panoUV = PanoSphereToSquare(dir);
    panoUV.x = frac(panoUV.x + cbv.Rotation);
    panoUV.y = 1 - panoUV.y;

    float3 color = gPano.SampleLevel(gSampler, panoUV, 0.0f).rgb;

    gCubemap[DTid] = float4(color, 1);
}
