#ifndef H_CONSTRUCT_FIRST_HIT_H
#define H_CONSTRUCT_FIRST_HIT_H

#include "PathTracing/HitInfo/ApplyMaterialTextures.hlsli"
#include "PathTracing/Debug/Internal/OutputColorMacros.hlsli"
#include "Utils/Debug/Palette.h"

float3 DepthToWorldPos(uint2 pixelCoord, float depth)
{
    float2 pixelUV = (pixelCoord + 0.5f) * gSettings.TexelSize;

    float4 ndc = float4(
        pixelUV * 2.0 - 1.0,
        depth,
        1.0
    );
    ndc.y = -ndc.y;

    float4 viewPos = mul(gSettings.InvP, ndc);
    viewPos /= viewPos.w;

    float4 worldPos = mul(gSettings.InvV, float4(viewPos.xyz, 1.0));
    return worldPos.xyz;
}

void ReconstructPrimaryRayHit(
    float3 cameraPos,
    uint2 pixelCoord,

    Texture2D<uint> gbufferMaterialIdx,
    Texture2D<float4> gbufferNormals,
    Texture2D<float> gbufferDepth,
    Texture2D<float4> gbufferUvMv,
    StructuredBuffer<Material> megaBufferMaterials,

    out HitInfo hitInfo,
    out bool isMiss)
{
    hitInfo.IsEntering = true;

    uint matIdx = gbufferMaterialIdx[pixelCoord];
    if (matIdx == 0)
    {
        isMiss = true;
        return;
    }
    isMiss = false;
    matIdx = matIdx - 1;

    hitInfo.Mat = megaBufferMaterials[matIdx];

    hitInfo.UV = gbufferUvMv[pixelCoord].xy;
    hitInfo.Ns_ff = RemapUtoS(gbufferNormals[pixelCoord].xyz);
    hitInfo.Ng_ff = hitInfo.Ns_ff;

    float depth = gbufferDepth[pixelCoord];
    float3 worldPos = DepthToWorldPos(pixelCoord, depth);
    hitInfo.RayT = length(worldPos - cameraPos);

    hitInfo.SFrame = CreateShadingFrame(hitInfo.Ns_ff);

    ApplyMaterialTextures(hitInfo);

    {
        DBG_OUTPUT3(Palette(matIdx),              Hit_MaterialIdx);
        DBG_OUTPUT3(hitInfo.Ns_ff,                Hit_Normals);
        DBG_OUTPUT3(hitInfo.Ns_ff,                Hit_NormalShadedFF);
        DBG_OUTPUT3(hitInfo.Ng_ff,                Hit_NormalGeometric);
        DBG_OUTPUT3(hitInfo.Ng_ff,                Hit_NormalGeometricFF);
        DBG_OUTPUT3(hitInfo.SFrame.T,             Hit_Tangent);
        DBG_OUTPUT3(hitInfo.SFrame.B,             Hit_Bitangent);
        DBG_OUTPUT2(hitInfo.UV,                   Hit_UV);
        DBG_OUTPUT1(hitInfo.IsEntering,           Hit_IsEntering);
        DBG_OUTPUT1(hitInfo.RayT,                 Hit_RayT);

        DBG_OUTPUT3(SampleSceneTexture3(hitInfo.UV, hitInfo.Mat.TexIdxAlbedo, NAN),                     Tex_Albedo);
        DBG_OUTPUT3(SampleSceneTexture3(hitInfo.UV, hitInfo.Mat.TexIdxNormal, NAN),                     Tex_Normal);
        DBG_OUTPUT3(SampleSceneTexture3(hitInfo.UV, hitInfo.Mat.TexIdxEmissive, NAN),                   Tex_Emissive);
        DBG_OUTPUT1(SampleSceneTexture1(hitInfo.UV, hitInfo.Mat.TexIdxRoughness, NAN),                  Tex_Roughness);
        DBG_OUTPUT1(SampleSceneTexture1(hitInfo.UV, hitInfo.Mat.TexIdxMetallic, NAN),                   Tex_Metallic);

        DBG_OUTPUT3(hitInfo.Mat.Albedo.rgb,                                                   Mat_Albedo);
        DBG_OUTPUT1(hitInfo.Mat.Albedo.a,                                                     Mat_Opacity);
        DBG_OUTPUT1(hitInfo.Mat.EmissiveStrength,                                             Mat_EmissiveStrength);
        DBG_OUTPUT3(hitInfo.Mat.EmissiveColor,                                                Mat_EmissiveColor);
        DBG_OUTPUT3(hitInfo.Emission,                                                         Mat_Emission);
        DBG_OUTPUT1(hitInfo.Mat.TransmissionFactor,                                           Mat_TransmissionFactor);
        DBG_OUTPUT3(hitInfo.Mat.TransmissionColor,                                            Mat_TransmissionColor);
        DBG_OUTPUT1(hitInfo.Mat.Roughness,                                                    Mat_Roughness);
        DBG_OUTPUT1(hitInfo.Mat.Metallic,                                                     Mat_Metallic);
        DBG_OUTPUT1(hitInfo.Mat.SpecularFactor,                                               Mat_SpecularFactor);
        DBG_OUTPUT1(hitInfo.Mat.AnisoStrength,                                                Mat_AnisoStrength);
        DBG_OUTPUT1(hitInfo.Mat.IOR_N,                                                        Mat_IorN);
    }
}

#endif