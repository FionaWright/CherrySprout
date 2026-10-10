#ifndef H_SHADOW_RAY_H
#define H_SHADOW_RAY_H

#include "PathTracing/Utils.hlsli"
#include "PathTracing/HitInfo/ExtractUtils.hlsli"

#define SHADOW_RAY_TMIN_BIAS_COEF 1
#define SHADOW_RAY_TMAX_BIAS_COEF 1

void TraceRayShadow(float3 pos, float3 dir, float lightDistance, out float shadowFactor)
{
    RayQuery<RAY_FLAGS> q;

    RayDesc ray;
    ray.Origin = pos;
    ray.Direction = normalize(dir);
    ray.TMin = (SHADOW_RAY_TMIN_BIAS_COEF * EPSILON);
    ray.TMax = lightDistance - (SHADOW_RAY_TMAX_BIAS_COEF * EPSILON);

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

        InstanceData instance = gMegaBufferInstanceData[instanceIdx];
        Material mat = gMegaBufferMaterials[instance.MaterialIndex];

        float2 uv = ExtractUV(q, instance);

        float4 albedoSample = mat.TexIdxAlbedo == -1 ? 1.0f : gSceneTextures[mat.TexIdxAlbedo].Sample(gSamplerLinearClamp, uv);
        float alpha = albedoSample.w;

        if (alpha > EPSILON) // TODO: Change back to mult
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