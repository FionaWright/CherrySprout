#include "MicrofacetModels/MicrofacetUtils.hlsli"
#include "Utils/SharedUtils.h"
#include "Utils/Math/ShadingFrame.h"
#include "Utils/CBVs.h"
#include "Scene/Material.h"

struct VsOut
{
    float4 position : SV_POSITION;
    float2 uv : TEXCOORD0;
    float3 normal : TEXCOORD1;
    uint materialIdx : TEXCOORD2;
    float4 clipPos : TEXCOORD3;
    float4 prevClipPos : TEXCOORD4;
};

StructuredBuffer<Material> gMegaBufferMaterials : register(t1);

Texture2D<float4> gSceneTextures[] : register(t0, space1);

SamplerState gSampler : register(s0);

struct GBufferOut
{
    uint   MaterialIdx1       : SV_Target0;
    float4 Normals2_UV2       : SV_Target1;
	float4 MV2_Reserved2      : SV_Target2;
};

#define NORMALS_MAPS_ENABLED 1

float2 ComputeMotionVectors(float4 clipPos, float4 prevClipPos)
{
    float2 ndc = clipPos.xy / clipPos.w;
    float2 ndcPrev = prevClipPos.xy / prevClipPos.w;

    ndc.y = -ndc.y;
    ndcPrev.y = -ndcPrev.y;

    ndc = RemapStoU(ndc);
    ndcPrev = RemapStoU(ndcPrev);

    if (ndcPrev.x < 0.0f || ndcPrev.x > 1.0f || ndcPrev.y < 0.0f || ndcPrev.y > 1.0f)
        return float2(10.0f, 10.0f);

    return ndc - ndcPrev;
}

GBufferOut PSMain(VsOut input)
{
    GBufferOut output;

    float3 Ns = normalize(input.normal);

    float2 uv = float2(input.uv.x, 1 - input.uv.y);

    Material mat = gMegaBufferMaterials[input.materialIdx];

    if (NORMALS_MAPS_ENABLED && mat.TexIdxNormal != -1)
    {
        float3 bumpSample = gSceneTextures[mat.TexIdxNormal].Sample(gSampler, uv).rgb;
        bumpSample = RemapUtoS(bumpSample);
        bumpSample.y = -bumpSample.y; // DX-convention

        ShadingFrame bumpFrame = CreateShadingFrame(Ns);
        Ns = bumpFrame.ToWorld(bumpSample);
        Ns = normalize(Ns);
    }

    float2 mv = ComputeMotionVectors(input.clipPos, input.prevClipPos);

    Ns = RemapStoU(Ns);

    output.MaterialIdx1 = input.materialIdx + 1;
    output.Normals2_UV2 = float4(Ns.xy, uv);
    output.MV2_Reserved2 = float4(mv, 0, 0);
    return output;
}