#ifndef H_GET_HIT_INFO_H
#define H_GET_HIT_INFO_H

#include "Utils/Math/ShadingFrame.h"
#include "Utils/HlslUtils.hlsli"
#include "PathTracing/Flags/Internal/MethodsHlsl.hlsli"
#include "PathTracing/Utils.hlsli"

#include "Utils/Debug/Palette.h"

#include "PathTracing/Structs.h"
#include "PathTracing/HitInfo/ExtractUtils.hlsli"
#include "PathTracing/HitInfo/ApplyMaterialTextures.hlsli"

void GetHitInfo(inout RayQuery<RAY_FLAGS> q, out HitInfo hitInfo)
{
    uint instanceIdx = q.CommittedInstanceIndex();

    hitInfo.IsEntering = q.CommittedTriangleFrontFace() == 0;
    hitInfo.RayT = q.CommittedRayT();

    InstanceData instance = gMegaBufferInstanceData[instanceIdx];

    if (DEBUG_ENABLED(NaNTests))
    {
        if (instance.MaterialIndex == -1)
        {
            hitInfo.Ns_ff = NAN;
            hitInfo.Mat.Albedo = NAN;
            return;
        }
    }

    hitInfo.Mat = gMegaBufferMaterials[instance.MaterialIndex];

    float3 Ng, Ns;
    float2 uv;
    ExtractInterpolatedAttributes(q, instance, Ns, Ng, uv);

    hitInfo.UV = uv;

    ApplyNormalMap(hitInfo, Ns);

    hitInfo.Ng_ff = hitInfo.IsEntering ? Ng : -Ng;
    hitInfo.Ns_ff = hitInfo.IsEntering ? Ns : -Ns;
    hitInfo.SFrame = CreateShadingFrame(hitInfo.Ns_ff);

    ApplyMaterialTextures(hitInfo);

    // Debug
    {
        if (DEBUG_ENABLED(Checkerboard))
        {
            uint2 uvQuant = uint2(uv * 100);
            hitInfo.Mat.Albedo.xyz = ((uvQuant.x + uvQuant.y) % 2 == 0) ? 1.0f : 0.0f;
        }

        if (DEBUG_ENABLED(FurnaceTestHHE))
        {
            hitInfo.Mat.Albedo.xyz = 1.0f;
            hitInfo.Emission = 0.0f;
        }

        if (DEBUG_ENABLED(FurnaceTestHDR))
        {
            hitInfo.Mat.Albedo.xyz = 0.0f;
            hitInfo.Emission = 1.0f;
        }

        DBG_OUTPUT3(Palette(instanceIdx),                                                     Hit_InstanceIdx);
        DBG_OUTPUT3(Palette(instance.MaterialIndex),                                          Hit_MaterialIdx);
        DBG_OUTPUT3(Ns,                                                                       Hit_Normals);
        DBG_OUTPUT3(hitInfo.Ns_ff,                                                            Hit_NormalShadedFF);
        DBG_OUTPUT3(Ng,                                                                       Hit_NormalGeometric);
        DBG_OUTPUT3(hitInfo.Ng_ff,                                                            Hit_NormalGeometricFF);
        DBG_OUTPUT3(hitInfo.SFrame.T,                                                         Hit_Tangent);
        DBG_OUTPUT3(hitInfo.SFrame.B,                                                         Hit_Bitangent);
        DBG_OUTPUT2(hitInfo.UV,                                                               Hit_UV);
        DBG_OUTPUT1(hitInfo.IsEntering,                                                       Hit_IsEntering);
        DBG_OUTPUT1(hitInfo.RayT,                                                             Hit_RayT);

        DBG_OUTPUT3(sampleTexture3(hitInfo, hitInfo.Mat.TexIdxAlbedo, NAN),                     Tex_Albedo);
        DBG_OUTPUT3(sampleTexture3(hitInfo, hitInfo.Mat.TexIdxNormal, NAN),                     Tex_Normal);
        DBG_OUTPUT3(sampleTexture3(hitInfo, hitInfo.Mat.TexIdxEmissive, NAN),                   Tex_Emissive);
        DBG_OUTPUT1(sampleTexture1(hitInfo, hitInfo.Mat.TexIdxRoughness, NAN),                  Tex_Roughness);
        DBG_OUTPUT1(sampleTexture1(hitInfo, hitInfo.Mat.TexIdxMetallic, NAN),                   Tex_Metallic);

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