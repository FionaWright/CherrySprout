#ifndef H_GET_HIT_INFO_H
#define H_GET_HIT_INFO_H

#include "Utils/Math/ShadingFrame.h"
#include "Utils/HlslUtils.hlsli"
#include "PathTracing/Flags/MethodsHlsl.hlsli"

#include "Utils/Debug/Palette.hlsli"
#include "PathTracing/Debug/OutputColorMacros.hlsli"

#include "PathTracing/HitInfo.h"

#define NO_TEXTURE -1

float SampleTexture1(HitInfo hitInfo, int idx, float fallback)
{
    if (idx == NO_TEXTURE)
        return fallback;

    return gSceneTextures[idx].Sample(gSampler, hitInfo.UV).r;
}

float3 SampleTexture3(HitInfo hitInfo, int idx, float3 fallback)
{
    if (idx == NO_TEXTURE)
        return fallback;

    return gSceneTextures[idx].Sample(gSampler, hitInfo.UV).rgb;
}

float4 SampleTexture4(HitInfo hitInfo, int idx, float4 fallback)
{
    if (idx == NO_TEXTURE)
        return fallback;

    return gSceneTextures[idx].Sample(gSampler, hitInfo.UV);
}

void ApplyMaterialTextures(inout HitInfo hitInfo)
{
    float4 albedoSample = SampleTexture4(hitInfo, hitInfo.Mat.TexIdxAlbedo, 1);
    float roughnessSample = SampleTexture1(hitInfo, hitInfo.Mat.TexIdxRoughness, 1);
    float metallicSample = SampleTexture1(hitInfo, hitInfo.Mat.TexIdxMetallic, 0);
    //float3 emissionSample = SampleTexture3(hitInfo, hitInfo.Mat.TexIdxEmissive, 1);

    if (true) // TODO
        albedoSample.xyz = pow(albedoSample.xyz, 2.2f);

    hitInfo.Mat.Albedo.rgb *= albedoSample.rgb;
    hitInfo.Mat.Roughness *= roughnessSample;
    hitInfo.Mat.Metallic *= metallicSample;

    hitInfo.Li = hitInfo.Mat.EmissiveStrength * hitInfo.Mat.EmissiveColor;
}

void GetHitInfo(inout RayQuery<RAY_FLAGS> q, out HitInfo hitInfo)
{
    uint instanceIdx = q.CommittedInstanceIndex();
    uint primitiveIdx = q.CommittedPrimitiveIndex();
    float2 barycentrics = q.CommittedTriangleBarycentrics();

    hitInfo.IsEntering = q.CommittedTriangleFrontFace() != 0;
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

    if (FEATURE_ENABLED(NormalMaps) && hitInfo.Mat.TexIdxNormal != NO_TEXTURE)
    {
        float3 bumpSample = SampleTexture3(hitInfo, hitInfo.Mat.TexIdxNormal, float3(0, 1, 0));
        bumpSample = RemapUtoS(bumpSample);
        bumpSample.y = -bumpSample.y; // DX-convention

        ShadingFrame bumpFrame = CreateShadingFrame(Ns);
        Ns = bumpFrame.ToWorld(bumpSample);
    }

    hitInfo.Ng_ff = hitInfo.IsEntering ? -Ng : Ng;
    hitInfo.Ns_ff = hitInfo.IsEntering ? -Ns : Ns;
    hitInfo.SFrame = CreateShadingFrame(hitInfo.Ns_ff);

    ApplyMaterialTextures(hitInfo);

    DBG_OUTPUT3(Palette(instanceIdx),                                                     InstanceIdx);
    DBG_OUTPUT3(Palette(instance.MaterialIndex),                                          MaterialIdx);
    DBG_OUTPUT2(barycentrics,                                                             Barycentrics);
    DBG_OUTPUT3(Ns,                                                                       Normals);
    DBG_OUTPUT3(hitInfo.Ns_ff,                                                            NormalShadedFF);
    DBG_OUTPUT3(hitInfo.Ng_ff,                                                            NormalGeometricFF);
    DBG_OUTPUT3(hitInfo.SFrame.T,                                                         Tangent);
    DBG_OUTPUT3(hitInfo.SFrame.B,                                                         Bitangent);
    DBG_OUTPUT2(hitInfo.UV,                                                               UV);

    DBG_OUTPUT3(SampleTexture3(hitInfo, hitInfo.Mat.TexIdxAlbedo, 1),                     TexAlbedo);
    DBG_OUTPUT3(SampleTexture3(hitInfo, hitInfo.Mat.TexIdxNormal, float3(0, 1, 0)),       TexNormal);
    DBG_OUTPUT3(SampleTexture3(hitInfo, hitInfo.Mat.TexIdxEmissive, 1),                   TexEmissive);
    DBG_OUTPUT1(SampleTexture1(hitInfo, hitInfo.Mat.TexIdxRoughness, 1),                  TexRoughness);
    DBG_OUTPUT1(SampleTexture1(hitInfo, hitInfo.Mat.TexIdxMetallic, 0),                   TexMetallic);

    DBG_OUTPUT3(hitInfo.Mat.Albedo.rgb,                                                   Albedo);
    DBG_OUTPUT1(hitInfo.Mat.Albedo.a,                                                     Opacity);
    DBG_OUTPUT1(hitInfo.Mat.EmissiveStrength,                                             EmissiveStrength);
    DBG_OUTPUT3(hitInfo.Mat.EmissiveColor,                                                EmissiveColor);
    DBG_OUTPUT3(hitInfo.Li,                                                               Emission);
    DBG_OUTPUT1(hitInfo.Mat.TransmissionFactor,                                           TransmissionFactor);
    DBG_OUTPUT3(hitInfo.Mat.TransmissionColor,                                            TransmissionColor);
    DBG_OUTPUT1(hitInfo.Mat.Roughness,                                                    Roughness);
    DBG_OUTPUT1(hitInfo.Mat.Metallic,                                                     Metallic);
    DBG_OUTPUT3(hitInfo.Mat.SpecularFactor,                                               SpecularFactor);
    DBG_OUTPUT1(hitInfo.Mat.AnisoStrength,                                                AnisoStrength);
    DBG_OUTPUT1(hitInfo.Mat.IOR_N,                                                        IorN);
}

#endif