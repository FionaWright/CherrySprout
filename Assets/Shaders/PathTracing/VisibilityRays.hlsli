#ifndef H_VISIBILITY_RAYS_H
#define H_VISIBILITY_RAYS_H

#include "PathTracing/Utils.hlsli"
#include "PathTracing/HitInfo/ExtractUtils.hlsli"

#define TMIN_BIAS_COEF 1
#define TMAX_BIAS_COEF 1

void TestVisibilityPoints(float3 source, float3 dest, out bool occluded)
{
    RayQuery<RAY_FLAGS> q;

    float distance = length(dest - source);
    float3 dir = normalize(dest - source);

    RayDesc ray;
    ray.Origin = source;
    ray.Direction = dir;
    ray.TMin = (TMIN_BIAS_COEF * EPSILON);
    ray.TMax = distance - (TMAX_BIAS_COEF * EPSILON);

    q.TraceRayInline(gTLAS, RAY_FLAGS, 0xFF, ray);
    q.Proceed();

    occluded = q.CommittedStatus() == COMMITTED_TRIANGLE_HIT;
}

void TestVisibilityDirection(float3 source, float3 dir, out bool occluded)
{
    RayQuery<RAY_FLAGS> q;

    RayDesc ray;
    ray.Origin = source;
    ray.Direction = dir;
    ray.TMin = TMIN_BIAS_COEF * EPSILON;
    ray.TMax = INF;

    q.TraceRayInline(gTLAS, RAY_FLAGS, 0xFF, ray);
    q.Proceed();

    occluded = q.CommittedStatus() == COMMITTED_TRIANGLE_HIT;
}

void TraceRayShadow(float3 source, float3 dir, float lightDistance, out float shadowFactor)
{
    RayQuery<RAY_FLAGS> q;

    RayDesc ray;
    ray.Origin = source;
    ray.Direction = normalize(dir);
    ray.TMin = (TMIN_BIAS_COEF * EPSILON);
    ray.TMax = lightDistance - (TMAX_BIAS_COEF * EPSILON);

    float3 lightPos = source + ray.Direction * lightDistance;

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