#ifndef H_SHADOW_RAY_H
#define H_SHADOW_RAY_H

#include "PathTracing/Utils.hlsli"

void TraceRayShadow(float3 pos, float3 dir, float lightDistance, out float shadowFactor)
{
    RayQuery<RAY_FLAGS> q;

    RayDesc ray;
    ray.Origin = pos;
    ray.Direction = dir;
    ray.TMin = 0.001;
    ray.TMax = lightDistance;

    shadowFactor = 1.0f;

    // TODO: Alpha handling

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

        // TODO: Transmission eval? 
        shadowFactor *= mat.TransmissionFactor;
        //shadowFactor *= Luminance(mat.TransmissionColor);

        if (shadowFactor <= 0.0f)
            return;

        float rayT = q.CommittedRayT();
        float advance = rayT + EPSILON;
        ray.Origin += ray.Direction * advance;
        ray.TMax -= advance;

        if (ray.TMax <= 0.0f)
            return;
    }
}

#endif