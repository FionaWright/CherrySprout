#ifndef H_SHADOW_RAY_H
#define H_SHADOW_RAY_H

#include "PathTracing/Utils.hlsli"
#include "PathTracing/HitInfo/ExtractUtils.hlsli"

void TraceRayShadow(float3 pos, float3 dir, float lightDistance, uint lightIdx, out float shadowFactor)
{
    RayQuery<RAY_FLAGS> q;

    RayDesc ray;
    ray.Origin = pos;
    ray.Direction = normalize(dir);
    ray.TMin = 0.001;
    ray.TMax = lightDistance - EPSILON;

    float3 lightPos = pos + ray.Direction * lightDistance;

    shadowFactor = 1.0f;

    for (uint i = 0; i < gSettings.MaxShadowRayDepth; i++)
    {
        q.TraceRayInline(gTLAS, RAY_FLAGS, 0xFF, ray);
        q.Proceed();

        bool isMiss = q.CommittedStatus() != COMMITTED_TRIANGLE_HIT;
        if (isMiss)
            return;

        uint instanceIdx = q.CommittedInstanceIndex();

        if (lightIdx >= gSettings.LsdBaseEmissives)
        {
            int emissiveIdx = gInstanceToEmissiveInstanceMap[instanceIdx];
            uint hitLightIdx = gSettings.LsdBaseEmissives + emissiveIdx;
            if (lightIdx == hitLightIdx)
                return;
        }

        InstanceData instance = gMegaBufferInstanceData[instanceIdx];
        Material mat = gMegaBufferMaterials[instance.MaterialIndex];

        float2 uv = ExtractUV(q, instance);

        float4 albedoSample = mat.TexIdxAlbedo == -1 ? 1.0f : gSceneTextures[mat.TexIdxAlbedo].Sample(gSamplerLinearClamp, uv);
        float alpha = albedoSample.w;

        if (alpha > EPSILON)
        {
            shadowFactor = 0.0f;
            return;
        }

        float rayT = q.CommittedRayT();
        ray.Origin += ray.Direction * (rayT + EPSILON);

        float3 toLight = lightPos - ray.Origin;
        float remainingDistance = length(toLight);
        if (remainingDistance <= EPSILON)
            return;
        ray.TMax = remainingDistance - EPSILON;
    }
}

#endif