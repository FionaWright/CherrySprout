#ifndef H_TRACE_H
#define H_TRACE_H

#include "PathTracing/3_Hit.hlsli"
#include "PathTracing/3_Miss.hlsli"

#include "Utils/Random.h"

float3 Trace(inout RayQuery<RAY_FLAGS> q, inout RayDesc ray, inout RngInfo rngInfo)
{
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

        float3 wo = -ray.Direction;

        Hit(q, ray, Lo, throughput, rngInfo, wo);
        //return throughput;
        //return ray.Direction;
    }

    return Lo;
}

#endif