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
    float3 beta = float3(1, 1, 1);

    for (uint i = 0; i <= gSettings.MaxRayDepth; i++)
    {
        q.TraceRayInline(gTLAS, RAY_FLAGS, 0xFF, ray);
        q.Proceed();

        if (q.CommittedStatus() != COMMITTED_TRIANGLE_HIT)
        {
            float3 L_sample = beta * Miss(ray.Origin, ray.Direction, i);

            Lo += L_sample;
            break;
        }

        float3 L_sample;
        Hit(q, ray, L_sample, beta, rngInfo);

        if (beta.x <= 0 && beta.y <= 0 && beta.z <= 0)
            break;

        Lo += L_sample;

        if (FEATURE_ENABLED(RussianRoulette) && i >= gSettings.RussianRouletteMinBounces)
        {
            float p = saturate(max(beta.r, max(beta.g, beta.b)));
            p = max(p, 0.05f);
            float rRR = Rand01(rngInfo);
            if (rRR > p)
                break;
            beta /= p;
        }
    }

    return Lo;
}

#endif