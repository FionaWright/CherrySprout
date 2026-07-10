#include "Utils/CBVs.h"
#include "Scene/InstanceData.h"

struct VsIn
{
    float3 position : POSITION;
    float3 normal : NORMAL;
    float2 uv : TEXCOORD0;
};

struct VsOut
{
    float4 position : SV_POSITION;
    float2 uv : TEXCOORD0;
    float3 normal : TEXCOORD1;
    uint materialIdx : TEXCOORD2;
};

ConstantBuffer<CbvGBufferPrePass_PerInstance> gInfo : register(b0); // Push Constants
ConstantBuffer<CbvMatrices_VP> gMatricesVP : register(b1);

StructuredBuffer<InstanceData> gMegaBufferInstanceData : register(t0);

VsOut VSMain(VsIn input)
{
    VsOut output;

    output.normal = normalize(mul((float3x3)gInfo.MTI, (float3)input.normal));
    output.uv = input.uv;

    float4 pos = float4(input.position, 1.0f);
    float4 worldPos = mul(gInfo.M, pos);

    pos = mul(gMatricesVP.V, worldPos);
    output.position = mul(gMatricesVP.P, pos);

    output.uv = input.uv;
    output.normal = normalize(mul((float3x3)gInfo.MTI, input.normal));

    InstanceData instanceData = gMegaBufferInstanceData[gInfo.InstanceIdx];
    output.materialIdx = instanceData.MaterialIndex;

    return output;
}