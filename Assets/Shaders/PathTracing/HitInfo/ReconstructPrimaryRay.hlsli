#ifndef H_CONSTRUCT_FIRST_HIT_H
#define H_CONSTRUCT_FIRST_HIT_H

#include "PathTracing/HitInfo/ApplyMaterialTextures.hlsli"

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
    hitInfo.IsEntering = false;

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
        DBG_OUTPUT3(Palette(matIdx), MaterialIdx);
        DBG_OUTPUT3(hitInfo.Ns_ff, Normals);
        DBG_OUTPUT3(hitInfo.Ns_ff, NormalShadedFF);
        DBG_OUTPUT3(hitInfo.Ng_ff, NormalGeometric);
        DBG_OUTPUT3(hitInfo.Ng_ff, NormalGeometricFF);
        DBG_OUTPUT3(hitInfo.SFrame.T, Tangent);
        DBG_OUTPUT3(hitInfo.SFrame.B, Bitangent);
        DBG_OUTPUT2(hitInfo.UV, UV);
        DBG_OUTPUT1(hitInfo.IsEntering, IsEntering);
        DBG_OUTPUT1(hitInfo.RayT, RayT);

        DBG_OUTPUT3(sampleTexture3(hitInfo, hitInfo.Mat.TexIdxAlbedo, NAN),                     TexAlbedo);
        DBG_OUTPUT3(sampleTexture3(hitInfo, hitInfo.Mat.TexIdxNormal, NAN),                     TexNormal);
        DBG_OUTPUT3(sampleTexture3(hitInfo, hitInfo.Mat.TexIdxEmissive, NAN),                   TexEmissive);
        DBG_OUTPUT1(sampleTexture1(hitInfo, hitInfo.Mat.TexIdxRoughness, NAN),                  TexRoughness);
        DBG_OUTPUT1(sampleTexture1(hitInfo, hitInfo.Mat.TexIdxMetallic, NAN),                   TexMetallic);

        DBG_OUTPUT3(hitInfo.Mat.Albedo.rgb,                                                   Albedo);
        DBG_OUTPUT1(hitInfo.Mat.Albedo.a,                                                     Opacity);
        DBG_OUTPUT1(hitInfo.Mat.EmissiveStrength,                                             EmissiveStrength);
        DBG_OUTPUT3(hitInfo.Mat.EmissiveColor,                                                EmissiveColor);
        DBG_OUTPUT3(hitInfo.Emission,                                                         Emission);
        DBG_OUTPUT1(hitInfo.Mat.TransmissionFactor,                                           TransmissionFactor);
        DBG_OUTPUT3(hitInfo.Mat.TransmissionColor,                                            TransmissionColor);
        DBG_OUTPUT1(hitInfo.Mat.Roughness,                                                    Roughness);
        DBG_OUTPUT1(hitInfo.Mat.Metallic,                                                     Metallic);
        DBG_OUTPUT3(hitInfo.Mat.SpecularFactor,                                               SpecularFactor);
        DBG_OUTPUT1(hitInfo.Mat.AnisoStrength,                                                AnisoStrength);
        DBG_OUTPUT1(hitInfo.Mat.IOR_N,                                                        IorN);
    }
}

#endif