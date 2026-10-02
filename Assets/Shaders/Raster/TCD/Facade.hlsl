#include "Utils/CBVs.h"
#include "MicrofacetModels/MicrofacetUtils.hlsli"
#include "Utils/SharedUtils.h"
#include "Utils/Math/ShadingFrame.h"
#include "Scene/Material.h"
#include "Utils/Debug/Palette.h"

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
};

ConstantBuffer<CbvForward_PerInstance> gCbvObject : register(b0, space0); // Push Constants
ConstantBuffer<CbvMatrices_VP> gMatricesVP : register(b1, space0);
ConstantBuffer<CbvForward> gSettings : register(b2, space0);

//Texture2D<float2> gBrdfInt : register(t0, space0);
//TextureCube gEnvMap : register(t0, space0);
StructuredBuffer<Material> gMaterials : register(t0, space0);
//TextureCube gIrradiance : register(t2, space0);

Texture2D<float4> gSceneTextures[] : register(t0, space1);

SamplerState gSampler : register(s0, space0);

VsOut VSMain(VsIn input)
{
    VsOut output;

    output.normal = normalize(mul((float3x3)gCbvObject.MTI, (float3)input.normal));
    output.uv = input.uv;

    float4 pos = float4(input.position, 1.0f);
    float4 worldPos = mul(gCbvObject.M, pos);
    pos = mul(gMatricesVP.V, worldPos);
    output.position = mul(gMatricesVP.P, pos);

    return output;
}

float4 PSMain(VsOut input) : SV_TARGET
{
    float3 Ng = normalize(input.normal);
    float2 uv = input.uv;
    uv.y *= gCbvObject.ScaleY;

    Material mat = gMaterials[gCbvObject.MaterialIdx];

    //float3 albedoSample = gSceneTextures[mat.TexIdxAlbedo].SampleLevel(gSampler, uv, 7).rgb;
    float3 albedoSample = gSceneTextures[mat.TexIdxAlbedo].SampleLevel(gSampler, uv, 7).rgb;

    if (Ng.y >= 0.9f)
        albedoSample.rgb = float3(0.3, 0.3, 0.3);

    float NdL = dot(Ng, normalize(-gSettings.DirLightDir));

    float ambient = 0.2f;
    float light = max(0.0f, NdL) + ambient;

    return light * float4(albedoSample, 1);
}