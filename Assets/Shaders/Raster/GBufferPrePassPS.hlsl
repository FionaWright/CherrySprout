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
};

StructuredBuffer<Material> gMegaBufferMaterials : register(t1);

Texture2D<float4> gSceneTextures[] : register(t0, space1);

SamplerState gSampler : register(s0);

struct GBufferOut
{
    uint   MaterialIdx1       : SV_Target0;
    float4 Normals3_Reserved1 : SV_Target1;
	float4 UV2_MV2            : SV_Target2;
};

GBufferOut PSMain(VsOut input)
{
    GBufferOut output;

    float3 Ns = normalize(input.normal);

    Material mat = gMegaBufferMaterials[input.materialIdx];

    if (mat.TexIdxNormal != -1)
    {
        float3 bumpSample = gSceneTextures[mat.TexIdxNormal].Sample(gSampler, input.uv).rgb;
        bumpSample = RemapUtoS(bumpSample);
        bumpSample.y = -bumpSample.y; // DX-convention

        ShadingFrame bumpFrame = CreateShadingFrame(Ns);
        Ns = bumpFrame.ToWorld(bumpSample);
    }

    output.MaterialIdx1 = input.materialIdx;
    output.Normals3_Reserved1 = float4(Ns, 0);
    output.UV2_MV2 = float4(input.uv, 0, 0);
    return output;
}