#ifndef H_SHADOW_RAY_H
#define H_SHADOW_RAY_H

#include "PathTracing/Utils.hlsli"
#include "PathTracing/HitInfo/ExtractUtils.hlsli"

void TraceRayShadow(float3 pos, float3 dir, float lightDistance, out float shadowFactor)
{
    RayQuery<RAY_FLAGS> q;

    RayDesc ray;
    ray.Origin = pos;
    ray.Direction = dir;
    ray.TMin = 0.001f;

    float3 lightPos = pos + dir * lightDistance;

    shadowFactor = 1.0f;

    for (uint i = 0; i < gSettings.MaxShadowRayDepth; i++)
    {
        float3 toLight = lightPos - ray.Origin;
        ray.TMax = length(toLight) - 5 * EPSILON;

        q.TraceRayInline(gTLAS, RAY_FLAGS, 0xFF, ray);
        q.Proceed();

        bool isMiss = q.CommittedStatus() != COMMITTED_TRIANGLE_HIT;
        if (isMiss)
            return;

        uint instanceIdx = q.CommittedInstanceIndex();
        InstanceData instance = gMegaBufferInstanceData[instanceIdx];
        Material mat = gMegaBufferMaterials[instance.MaterialIndex];

        float2 uv = ExtractUV(q, instance);

        float4 albedoSample = mat.TexIdxAlbedo == -1 ? 1.0f : gSceneTextures[mat.TexIdxAlbedo].Sample(gSamplerLinearWrap, uv);
        float alpha = albedoSample.w;

        shadowFactor *= 1.0f - alpha;

        if (shadowFactor <= 0.0f)
            return;

        float rayT = q.CommittedRayT();
        ray.Origin += ray.Direction * (rayT + EPSILON);

        float remainingDistance = dot(toLight, ray.Direction);
        if (remainingDistance <= 0.0f)
            return;;
    }
}

#endif