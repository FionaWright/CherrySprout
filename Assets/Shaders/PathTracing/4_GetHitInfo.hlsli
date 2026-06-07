#ifndef H_GET_HIT_INFO_H
#define H_GET_HIT_INFO_H

#include "Utils/ShadingFrame.h"

struct HitInfo
{
    Material Mat;
    float3 Albedo;
    float Opacity;

    float3 Ng_ff;
    float3 Ns;
    float3 Ns_ff;

    float3 Li;

    float2 AnisoDir;
    float AnisoStrength;

    float2 UV;
    bool Entering;
};

void GetHitInfo(inout RayQuery<RAY_FLAGS> q, out HitInfo hitInfo)
{
    uint instanceIdx = q.CommittedInstanceIndex();
    uint primitiveIdx = q.CommittedPrimitiveIndex();
    float2 barycentrics = q.CommittedTriangleBarycentrics();

    hitInfo.Entering = q.CommittedTriangleFrontFace() != 0;

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
    hitInfo.Ng_ff = hitInfo.Entering ? Ng : -Ng;

    hitInfo.Ns = v0.Normal * bary.x + v1.Normal * bary.y + v2.Normal * bary.z;
    hitInfo.Ns = normalize(mul((float3x3)instance.MTI, hitInfo.Ns));

    if (FEATURE_ENABLED(NormalMaps))
    {
        float3 bumpSample = gSceneTextures[hitInfo.Mat.TexIdxNormal].Sample(gSampler, hitInfo.UV).rgb * 2.0f - 1.0f;
        //bumpSample.y = -bumpSample.y; // DX-convention

        ShadingFrame bumpFrame = CreateShadingFrame(hitInfo.Ns);
        hitInfo.Ns = bumpFrame.ToWorld(bumpSample);
    }
    hitInfo.Ns_ff = hitInfo.Entering ? hitInfo.Ns : -hitInfo.Ns;

    float4 albedoSample = gSceneTextures[hitInfo.Mat.TexIdxAlbedo].Sample(gSampler, hitInfo.UV);
    if (true) // TODO
        albedoSample.xyz = pow(albedoSample.xyz, 2.2f);

    //float3 emissionSample = gSceneTextures[hitInfo.Mat.TexIdxEmissive].Sample(gSampler, hitInfo.UV).rgb;

    float roughnessSample = gSceneTextures[hitInfo.Mat.TexIdxRoughness].Sample(gSampler, hitInfo.UV).r;
    float metallicSample = gSceneTextures[hitInfo.Mat.TexIdxMetallic].Sample(gSampler, hitInfo.UV).r;
    hitInfo.Mat.Roughness *= roughnessSample;
    hitInfo.Mat.Metallic *= metallicSample;

    hitInfo.Albedo = float3(hitInfo.Mat.Albedo.rgb * albedoSample.rgb);
    //hitInfo.Albedo = albedoSample.rgb;
    hitInfo.Opacity = albedoSample.a;
    hitInfo.Li = hitInfo.Mat.EmissiveStrength * hitInfo.Mat.EmissiveColor;
}

#endif