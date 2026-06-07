#ifndef H_TRACE_H
#define H_TRACE_H

#define RAY_FLAGS RAY_FLAG_CULL_NON_OPAQUE|RAY_FLAG_SKIP_PROCEDURAL_PRIMITIVES

#include "PathTracing/3_Hit.hlsli"
#include "PathTracing/3_Miss.hlsli"

#include "Utils/Random.h"

float3 Trace(RayDesc ray, inout RngInfo rngInfo)
{
    RayQuery<RAY_FLAGS> q;

    float3 Lo = float3(0, 0, 0);
    float3 throughput = float3(1, 1, 1);

    for (uint i = 0; i <= gSettings.MaxRayDepth; i++)
    {
        q.TraceRayInline(gTLAS, RAY_FLAGS, 0xFF, ray);
        q.Proceed();

        if (q.CommittedStatus() != COMMITTED_TRIANGLE_HIT)
        {
            float3 L_sample = throughput * Miss(ray.Origin, ray.Direction, i);

            Lo += L_sample;
            break;
        }

        float3 L_sample;
        float3 dbgOutput;
        Hit(q, ray, L_sample, throughput, rngInfo, dbgOutput);

        Lo += L_sample;

        if (DEBUG_ENABLED(OutputColor))
        {
            return dbgOutput;
        }
    }

    return Lo;
}

#endif