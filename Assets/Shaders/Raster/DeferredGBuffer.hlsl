#include "Utils/CBVs.h"
#include "Scene/InstanceData.h"
#include "MicrofacetModels/MicrofacetUtils.hlsli"
#include "Utils/SharedUtils.h"
#include "Utils/HlslUtils.hlsli"
#include "Utils/Math/ShadingFrame.h"
#include "Scene/Material.h"

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
    float4 clipPos : TEXCOORD3;
    float4 prevClipPos : TEXCOORD4;
};

struct GBufferOut
{
    uint   MaterialIdx1                 : SV_Target0;
    float4 Normals2_UV2                 : SV_Target1;
    float4 MV2_Reserved2                : SV_Target2;
    float4 Emission3_Reserved1          : SV_Target3;
};

ConstantBuffer<CbvGBufferPrePass_PerInstance> gInfo : register(b0); // Push Constants
ConstantBuffer<CbvMatrices_VP> gMatricesVP : register(b1);

StructuredBuffer<InstanceData> gMegaBufferInstanceData : register(t0);
StructuredBuffer<Material> gMegaBufferMaterials : register(t1);

Texture2D<float4> gSceneTextures[] : register(t0, space1);

SamplerState gSamplerLinearWrap : register(s0);

#include "PathTracing/HitInfo/ApplyMaterialTextures.hlsli"

VsOut VSMain(VsIn input)
{
    VsOut output;

    InstanceData instanceData = gMegaBufferInstanceData[gInfo.InstanceIdx];

    float4 pos = float4(input.position, 1.0f);
    float4 worldPos = mul(instanceData.M, pos);

    pos = mul(gMatricesVP.V, worldPos);
    output.position = mul(gMatricesVP.P, pos);

    output.clipPos = output.position;
    output.prevClipPos = mul(gMatricesVP.P, mul(gMatricesVP.PrevV, worldPos));

    output.uv = input.uv;
    output.normal = normalize(mul((float3x3)instanceData.MTI, input.normal));

    output.materialIdx = instanceData.MaterialIndex;

    return output;
}

GBufferOut PSMain(VsOut input)
{
    GBufferOut output;

    float2 uv = float2(input.uv.x, 1 - input.uv.y);

    Material mat = gMegaBufferMaterials[input.materialIdx];

    float3 Ns = normalize(input.normal);
    ApplyNormalMap(uv, mat.TexIdxNormal, Ns, true);
    float2 Ns_oct = EncodeNormalOctahedral(Ns);

    float2 mv = ComputeMotionVectors(input.clipPos, input.prevClipPos);

    float3 emissionSample = SampleSceneTexture3(uv, mat.TexIdxEmissive, 1);
    emissionSample = SRGB_to_LRGB_Fast(emissionSample);
    float3 emission = mat.EmissiveStrength * mat.EmissiveColor * emissionSample;

    output.MaterialIdx1 = input.materialIdx + 1;
    output.Normals2_UV2 = float4(Ns_oct, uv);
    output.MV2_Reserved2 = float4(mv, 0, 0);
    output.Emission3_Reserved1 = float4(emission, 1.0f);
    return output;
}