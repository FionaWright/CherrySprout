#ifndef H_GET_HIT_INFO_H
#define H_GET_HIT_INFO_H

#include "Utils/Math/ShadingFrame.h"
#include "Utils/HlslUtils.hlsli"
#include "PathTracing/Flags/MethodsHlsl.hlsli"
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

        DBG_OUTPUT3(Palette(instanceIdx),                                                     InstanceIdx);
        DBG_OUTPUT3(Palette(instance.MaterialIndex),                                          MaterialIdx);
        DBG_OUTPUT3(Ns,                                                                       Normals);
        DBG_OUTPUT3(hitInfo.Ns_ff,                                                            NormalShadedFF);
        DBG_OUTPUT3(Ng,                                                                       NormalGeometric);
        DBG_OUTPUT3(hitInfo.Ng_ff,                                                            NormalGeometricFF);
        DBG_OUTPUT3(hitInfo.SFrame.T,                                                         Tangent);
        DBG_OUTPUT3(hitInfo.SFrame.B,                                                         Bitangent);
        DBG_OUTPUT2(hitInfo.UV,                                                               UV);
        DBG_OUTPUT1(hitInfo.IsEntering,                                                       IsEntering);
        DBG_OUTPUT1(hitInfo.RayT,                                                             RayT);

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
        DBG_OUTPUT1(hitInfo.Mat.SpecularFactor,                                               SpecularFactor);
        DBG_OUTPUT1(hitInfo.Mat.AnisoStrength,                                                AnisoStrength);
        DBG_OUTPUT1(hitInfo.Mat.IOR_N,                                                        IorN);
    }
}

#endif