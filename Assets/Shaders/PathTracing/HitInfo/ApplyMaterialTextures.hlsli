#ifndef H_APPLY_MATERIAL_TEXTURES_H
#define H_APPLY_MATERIAL_TEXTURES_H

#include "Utils/Math/ShadingFrame.h"
#include "Utils/HlslUtils.hlsli"

#include "PathTracing/Utils.hlsli"
#include "PathTracing/Structs.h"

#define NO_TEXTURE -1

float SampleSceneTexture1(float2 uv, int idx, float fallback)
{
    if (idx == NO_TEXTURE)
        return fallback;

    return gSceneTextures[idx].Sample(gSamplerLinearWrap, uv).r;
}

float3 SampleSceneTexture3(float2 uv, int idx, float3 fallback)
{
    if (idx == NO_TEXTURE)
        return fallback;

    return gSceneTextures[idx].Sample(gSamplerLinearWrap, uv).rgb;
}

float4 SampleSceneTexture4(float2 uv, int idx, float4 fallback)
{
    if (idx == NO_TEXTURE)
        return fallback;

    return gSceneTextures[idx].Sample(gSamplerLinearWrap, uv);
}

void ApplyMaterialTextures(inout HitInfo hitInfo)
{
    float4 albedoSample =   SampleSceneTexture4(hitInfo.UV, hitInfo.Mat.TexIdxAlbedo, 1);
    float roughnessSample = SampleSceneTexture1(hitInfo.UV, hitInfo.Mat.TexIdxRoughness, 1);
    float metallicSample =  SampleSceneTexture1(hitInfo.UV, hitInfo.Mat.TexIdxMetallic, 0);
    float3 emissionSample = SampleSceneTexture3(hitInfo.UV, hitInfo.Mat.TexIdxEmissive, 1);

    albedoSample.xyz = SRGB_to_LRGB(albedoSample.xyz);
    emissionSample.xyz = SRGB_to_LRGB(emissionSample.xyz);

    hitInfo.Mat.Albedo *= albedoSample;
    hitInfo.Mat.Roughness *= roughnessSample;
    hitInfo.Mat.Metallic *= metallicSample;

    if (FEAT_CORE(Emission))
        hitInfo.Emission = hitInfo.Mat.EmissiveStrength * hitInfo.Mat.EmissiveColor * emissionSample.rgb;
    else
        hitInfo.Emission = 0.0f;
}

void ApplyNormalMap(float2 uv, uint texIdxNormal, inout float3 Ns)
{
    if (!FEAT_CORE(NormalMaps))
        return;

    if (texIdxNormal == NO_TEXTURE)
        return;

    float3 bumpSample = gSceneTextures[texIdxNormal].Sample(gSamplerLinearWrap, uv).rgb;
    bumpSample = RemapUtoS(bumpSample);
    bumpSample.y = -bumpSample.y; // DX-convention

    ShadingFrame bumpFrame = CreateShadingFrame(Ns);
    Ns = bumpFrame.ToWorld(bumpSample);
}

#endif