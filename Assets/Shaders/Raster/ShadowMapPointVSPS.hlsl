#include "Utils/CBVs.h"
#include "Utils/SharedUtils.h"
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
    float4 worldPos : TEXCOORD1;
};

ConstantBuffer<CbvShadowMap_PerInstance> gCbvObject : register(b0, space0); // Push Constants
ConstantBuffer<CbvShadowMapCreate> gMatricesVP : register(b1, space0);

StructuredBuffer<Material> gMaterials : register(t0, space0);

Texture2D<float4> gSceneTextures[] : register(t0, space1);

SamplerState gSamplerLinearWrap : register(s0, space0);

#include "PathTracing/HitInfo/ApplyMaterialTextures.hlsli"

VsOut VSMain(VsIn input)
{
    VsOut output;

    output.uv = input.uv;

    float4 pos = float4(input.position, 1.0f);
    output.worldPos = mul(gCbvObject.M, pos);
    pos = mul(gMatricesVP.V, output.worldPos);
    output.position = mul(gMatricesVP.P, pos);

    return output;
}

float PSMain(VsOut input) : SV_DEPTH
{
    {
        float2 uv = input.uv;
        uv.y = 1 - uv.y;

        HitInfo hitInfo;
        hitInfo.Mat = gMaterials[gCbvObject.MaterialIdx];
        hitInfo.UV = uv;

        ApplyMaterialTextures(hitInfo);

        clip(hitInfo.Mat.Albedo.a < 0.5f);
    }

    float depth = length(input.worldPos.xyz - gMatricesVP.PointLightPosition);

    return depth / gMatricesVP.FarPlane;
}