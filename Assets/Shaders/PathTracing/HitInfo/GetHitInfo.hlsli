#ifndef H_GET_HIT_INFO_H
#define H_GET_HIT_INFO_H

#include "Utils/Math/ShadingFrame.h"
#include "Utils/HlslUtils.hlsli"
#include "PathTracing/Flags/MethodsHlsl.hlsli"
#include "PathTracing/Utils.hlsli"

#include "Utils/Debug/Palette.h"

#include "PathTracing/Structs.h"
#include "PathTracing/HitInfo/ApplyMaterialTextures.hlsli"

void GetHitInfo(inout RayQuery<RAY_FLAGS> q, out HitInfo hitInfo)
{
    uint instanceIdx = q.CommittedInstanceIndex();
    uint primitiveIdx = q.CommittedPrimitiveIndex();
    float2 barycentrics = q.CommittedTriangleBarycentrics();

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

    uint primitiveOffset = instance.MegaBufferOffsetIndex / 3; // TODO: Put primitiveOffset in instanceData?
    uint3 tri = gMegaBufferIndex[primitiveOffset + primitiveIdx];
    Vertex v0 = gMegaBufferVertex[instance.MegaBufferOffsetVertex + tri.x];
    Vertex v1 = gMegaBufferVertex[instance.MegaBufferOffsetVertex + tri.y];
    Vertex v2 = gMegaBufferVertex[instance.MegaBufferOffsetVertex + tri.z];

    precise float3 bary = float3(1 - barycentrics.x - barycentrics.y, barycentrics.x, barycentrics.y);

    hitInfo.UV = v0.UV * bary.x + v1.UV * bary.y + v2.UV * bary.z;
    hitInfo.UV.y = 1 - hitInfo.UV.y;

	float3 p0 = mul(instance.M, float4(v0.Position,1)).xyz;
	float3 p1 = mul(instance.M, float4(v1.Position,1)).xyz;
	float3 p2 = mul(instance.M, float4(v2.Position,1)).xyz;
	float3 Ng = normalize( cross(p1 - p0, p2 - p0) );

    float3 Ns = v0.Normal * bary.x + v1.Normal * bary.y + v2.Normal * bary.z;
    Ns = normalize(mul((float3x3)instance.MTI, Ns));

    ApplyNormalMap(hitInfo, Ns);

    hitInfo.Ng_ff = hitInfo.IsEntering ? Ng : -Ng;
    hitInfo.Ns_ff = hitInfo.IsEntering ? Ns : -Ns;
    hitInfo.SFrame = CreateShadingFrame(hitInfo.Ns_ff);

    ApplyMaterialTextures(hitInfo);

    // Debug
    {
        DBG_OUTPUT3(Palette(instanceIdx),                                                     InstanceIdx);
        DBG_OUTPUT3(Palette(instance.MaterialIndex),                                          MaterialIdx);
        DBG_OUTPUT2(barycentrics,                                                             Barycentrics);
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
        DBG_OUTPUT3(hitInfo.Mat.SpecularFactor,                                               SpecularFactor);
        DBG_OUTPUT1(hitInfo.Mat.AnisoStrength,                                                AnisoStrength);
        DBG_OUTPUT1(hitInfo.Mat.IOR_N,                                                        IorN);
    }
}

#endif