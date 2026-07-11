#ifndef H_TRACE_H
#define H_TRACE_H

#define RAY_FLAGS RAY_FLAG_CULL_NON_OPAQUE|RAY_FLAG_SKIP_PROCEDURAL_PRIMITIVES

#include "PathTracing/Structs.h"
#include "PathTracing/3_Hit.hlsli"
#include "PathTracing/3_Miss.hlsli"
#include "PathTracing/Debug/PathDumper.hlsli"

#include "Utils/Random.h"

float3 Trace(RayDesc ray, inout RngInfo rngInfo, uint2 pixelCoord)
{
    RayQuery<RAY_FLAGS> q;

    float3 Lo = float3(0, 0, 0);
    float3 beta = float3(1, 1, 1);

    PathState pathState;
    pathState.LastRayDiracDelta = false;

    for (uint i = 0; i < gSettings.MaxRayDepth; i++)
    {
        q.TraceRayInline(gTLAS, RAY_FLAGS, 0xFF, ray);
        q.Proceed();

        pathState.RaySegmentIdx = i;
        DBG_SET_CURRENT_RAY_DEPTH(i);

        if (q.CommittedStatus() != COMMITTED_TRIANGLE_HIT)
        {
            float3 L_sample = beta * Miss(pathState, ray.Origin, ray.Direction, i);

            if (FEATURE_ENABLED(FireflyThreshold))
            {
                float L_lum = Luminance(L_sample);
                if (L_lum > gSettings.FireflyThreshold)
                    L_sample *= gSettings.FireflyThreshold / L_lum;
            }

            Lo += L_sample;
            break;
        }

        pathState.LastRayDiracDelta = false;

        float3 L_sample;
        Hit(q, ray, pathState, L_sample, beta, rngInfo, pixelCoord);

        DBG_PATH_DUMP_PATH_STATE(pathState);

        if (beta.x <= 0 && beta.y <= 0 && beta.z <= 0)
            break;

        if (FEATURE_ENABLED(FireflyThreshold))
        {
            float L_lum = Luminance(L_sample);
            if (L_lum > gSettings.FireflyThreshold)
                L_sample *= gSettings.FireflyThreshold / L_lum;
        }

        Lo += L_sample; // TODO: Should the L_sample * beta be moved out here?

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