#ifndef H_GET_HIT_INFO_H
#define H_GET_HIT_INFO_H

#include "Utils/Math/ShadingFrame.h"
#include "Utils/HlslUtils.hlsli"
#include "PathTracing/Flags.h"

#include "Utils/Debug/Palette.hlsli"
#include "PathTracing/Debug/OutputColor.h"

struct HitInfo
{
    Material Mat;

    float3 Ng_ff;
    float3 Ns_ff;
    ShadingFrame SFrame;

    float3 Li;

    float2 AnisoDir;
    float AnisoStrength;

    float2 UV;
    bool IsEntering;
    float RayT;
};

float3 SampleTexture(HitInfo hitInfo, int idx, float3 fallback)
{
    if (idx == -1)
        return fallback;

    return gSceneTextures[idx].Sample(gSampler, hitInfo.UV).rgb;
}

float4 SampleTexture(HitInfo hitInfo, int idx, float4 fallback)
{
    if (idx == -1)
        return fallback;

    return gSceneTextures[idx].Sample(gSampler, hitInfo.UV);
}

void GetHitInfo(inout RayQuery<RAY_FLAGS> q, out HitInfo hitInfo)
{
    uint instanceIdx = q.CommittedInstanceIndex();
    uint primitiveIdx = q.CommittedPrimitiveIndex();
    float2 barycentrics = q.CommittedTriangleBarycentrics();

    hitInfo.IsEntering = q.CommittedTriangleFrontFace() != 0;
    hitInfo.RayT = q.CommittedRayT();

    InstanceData instance = gMegaBufferInstanceData[instanceIdx];
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

    if (FEATURE_ENABLED(NormalMaps))
    {
        float3 bumpSample = SampleTexture(hitInfo, hitInfo.Mat.TexIdxNormal, float3(0, 1, 0));
        bumpSample = RemapUtoS(bumpSample);
        bumpSample.y = -bumpSample.y; // DX-convention

        ShadingFrame bumpFrame = CreateShadingFrame(Ns);
        Ns = bumpFrame.ToWorld(bumpSample);
    }

    hitInfo.Ng_ff = hitInfo.IsEntering ? Ng : -Ng;
    hitInfo.Ns_ff = hitInfo.IsEntering ? Ns : -Ns;
    hitInfo.SFrame = CreateShadingFrame(hitInfo.Ns_ff);

    float4 albedoSample = gSceneTextures[hitInfo.Mat.TexIdxAlbedo].Sample(gSampler, hitInfo.UV);
    if (true) // TODO
        albedoSample.xyz = pow(albedoSample.xyz, 2.2f);
    hitInfo.Mat.Albedo.rgb *= albedoSample.rgb;

    //float3 emissionSample = gSceneTextures[hitInfo.Mat.TexIdxEmissive].Sample(gSampler, hitInfo.UV).rgb;

    float roughnessSample = gSceneTextures[hitInfo.Mat.TexIdxRoughness].Sample(gSampler, hitInfo.UV).r;
    float metallicSample = gSceneTextures[hitInfo.Mat.TexIdxMetallic].Sample(gSampler, hitInfo.UV).r;
    hitInfo.Mat.Roughness *= roughnessSample;
    hitInfo.Mat.Metallic *= metallicSample;

    hitInfo.Li = hitInfo.Mat.EmissiveStrength * hitInfo.Mat.EmissiveColor;

    DBG_OUTPUT3_FORCE(gSceneTextures[hitInfo.Mat.TexIdxAlbedo].Sample(gSampler, hitInfo.UV).rgb);

    DBG_OUTPUT3(Palette(instanceIdx),                                               InstanceIdx);
    DBG_OUTPUT3(Palette(instance.MaterialIndex),                                    MaterialIdx);
    DBG_OUTPUT2(barycentrics,                                                       Barycentrics);
    DBG_OUTPUT3(SampleTexture(hitInfo, hitInfo.Mat.TexIdxNormal, float3(0, 1, 0)),  NormalMap);
    DBG_OUTPUT3(hitInfo.Ns_ff,                                                      NormalShadedFF);
    DBG_OUTPUT3(hitInfo.Ng_ff,                                                      NormalGeometricFF);
    DBG_OUTPUT2(hitInfo.UV,                                                         UV);

    DBG_OUTPUT3(hitInfo.Mat.Albedo.rgb,              Albedo);
    DBG_OUTPUT1(hitInfo.Mat.Albedo.a,                Opacity);
    DBG_OUTPUT1(hitInfo.Mat.EmissiveStrength,        EmissiveStrength);
    DBG_OUTPUT3(hitInfo.Mat.EmissiveColor,           EmissiveColor);
    DBG_OUTPUT3(hitInfo.Li,                          Emission);
    DBG_OUTPUT1(hitInfo.Mat.TransmissionFactor,      TransmissionFactor);
    DBG_OUTPUT3(hitInfo.Mat.TransmissionColor,       TransmissionColor);
    DBG_OUTPUT1(hitInfo.Mat.Roughness,               Roughness);
    DBG_OUTPUT1(hitInfo.Mat.Metallic,                Metallic);
    DBG_OUTPUT3(hitInfo.Mat.SpecularFactor,          SpecularFactor);
    DBG_OUTPUT1(hitInfo.Mat.AnisoStrength,           AnisoStrength);
    DBG_OUTPUT1(hitInfo.Mat.IOR_N,                   IorN);
}

#endif