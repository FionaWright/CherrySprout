#ifndef H_APPLY_MATERIAL_TEXTURES_H
#define H_APPLY_MATERIAL_TEXTURES_H

#include "Utils/Math/ShadingFrame.h"
#include "Utils/HlslUtils.hlsli"

#include "PathTracing/Utils.hlsli"
#include "PathTracing/Structs.h"

#define NO_TEXTURE -1

float sampleTexture1(HitInfo hitInfo, int idx, float fallback)
{
    if (idx == NO_TEXTURE)
        return fallback;

    return gSceneTextures[idx].Sample(gSampler, hitInfo.UV).r;
}

float3 sampleTexture3(HitInfo hitInfo, int idx, float3 fallback)
{
    if (idx == NO_TEXTURE)
        return fallback;

    return gSceneTextures[idx].Sample(gSampler, hitInfo.UV).rgb;
}

float4 sampleTexture4(HitInfo hitInfo, int idx, float4 fallback)
{
    if (idx == NO_TEXTURE)
        return fallback;

    return gSceneTextures[idx].Sample(gSampler, hitInfo.UV);
}

void ApplyMaterialTextures(inout HitInfo hitInfo)
{
    float4 albedoSample = sampleTexture4(hitInfo, hitInfo.Mat.TexIdxAlbedo, 1);
    float roughnessSample = sampleTexture1(hitInfo, hitInfo.Mat.TexIdxRoughness, 1);
    float metallicSample = sampleTexture1(hitInfo, hitInfo.Mat.TexIdxMetallic, 0);
    float3 emissionSample = sampleTexture3(hitInfo, hitInfo.Mat.TexIdxEmissive, 1);

    albedoSample.xyz = SRGB_to_LRGB(albedoSample.xyz);
    emissionSample.xyz = SRGB_to_LRGB(emissionSample.xyz);

    hitInfo.Mat.Albedo *= albedoSample;
    hitInfo.Mat.Roughness *= roughnessSample;
    hitInfo.Mat.Metallic *= metallicSample;

    if (FEATURE_ENABLED(Emission))
        hitInfo.Emission = hitInfo.Mat.EmissiveStrength * hitInfo.Mat.EmissiveColor * emissionSample.rgb;
    else
        hitInfo.Emission = 0.0f;
}

void ApplyNormalMap(inout HitInfo hitInfo, inout float3 Ns)
{
    if (!FEATURE_ENABLED(NormalMaps))
        return;

    if (hitInfo.Mat.TexIdxNormal == NO_TEXTURE)
        return;

    float3 bumpSample = sampleTexture3(hitInfo, hitInfo.Mat.TexIdxNormal, float3(0, 1, 0));
    bumpSample = RemapUtoS(bumpSample);
    bumpSample.y = -bumpSample.y; // DX-convention

    ShadingFrame bumpFrame = CreateShadingFrame(Ns);
    Ns = bumpFrame.ToWorld(bumpSample);
}

#endif