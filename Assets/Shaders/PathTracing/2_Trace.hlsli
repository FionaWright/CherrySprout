#ifndef H_TRACE_H
#define H_TRACE_H

#include "PathTracing/Structs.h"
#include "PathTracing/Utils.hlsli"
#include "PathTracing/3_Hit.hlsli"
#include "PathTracing/3_Miss.hlsli"
#include "PathTracing/HitInfo/ReconstructPrimaryRay.hlsli"
#include "PathTracing/Debug/PathDumper.hlsli"

#include "Utils/Random.h"

float3 Trace(float3 origin, float3 dir, inout RngInfo rngInfo, uint2 pixelCoord)
{
    RayQuery<RAY_FLAGS> q;

    PathState pathState = CreatePathState(origin, dir);

    for (uint i = 0; i < gSettings.MaxRayDepth; i++)
    {
        pathState.RaySegmentIdx = i;
        pathState.LastRayWasDiracDelta = false;

        bool isMiss;
        HitInfo hitInfo;

        if (FEATURE_ENABLED(ReconstructPrimaryRay) && i == 0)
        {
            ReconstructPrimaryRayHit(
                gSettings.CameraPositionWorld,
                pixelCoord,
                gGBufferMaterialIdx,
                gGBufferNormals,
                gGBufferDepth,
                gGBufferUvMv,
                gMegaBufferMaterials,
                hitInfo,
                isMiss
            );
        }
        else
        {
            q.TraceRayInline(gTLAS, RAY_FLAGS, 0xFF, pathState.Desc);
            q.Proceed();

            isMiss = q.CommittedStatus() != COMMITTED_TRIANGLE_HIT;

            if (!isMiss)
                GetHitInfo(q, hitInfo);
        }

        if (isMiss)
        {
            float3 L_sample = pathState.Beta * Miss(pathState, i);

            if (FEATURE_ENABLED(FireflyThreshold))
            {
                float L_lum = Luminance(L_sample);
                if (L_lum > gSettings.FireflyThreshold)
                    L_sample *= gSettings.FireflyThreshold / L_lum;
            }

            pathState.Lo += L_sample;
            break;
        }

        float3 L_sample;
        Hit(hitInfo, pathState, L_sample, rngInfo, pixelCoord);

        // Debug
        {
            DBG_SET_CURRENT_RAY_DEPTH(i);
            DBG_PATH_DUMP_MARK_EXPLORED();
            DBG_OUTPUT3(Palette(gGBufferMaterialIdx[pixelCoord] - 1),           GBufferMatIdx);
            DBG_OUTPUT3(gGBufferNormals[pixelCoord].rgb,                        GBufferNormalsUnorm);
            DBG_OUTPUT1(gGBufferDepth[pixelCoord].r,                            GBufferDepth);
            DBG_OUTPUT2(gGBufferUvMv[pixelCoord].rg,                            GBufferUv);
            DBG_OUTPUT2(gGBufferUvMv[pixelCoord].ba,                            GBufferMv);
            DBG_PATH_DUMP_PATH_STATE(pathState);
        }

        if (pathState.Beta.x <= 0 && pathState.Beta.y <= 0 && pathState.Beta.z <= 0)
            break;

        if (FEATURE_ENABLED(FireflyThreshold))
        {
            float L_lum = Luminance(L_sample);
            if (L_lum > gSettings.FireflyThreshold)
                L_sample *= gSettings.FireflyThreshold / L_lum;
        }

        pathState.Lo += L_sample; // TODO: Should the L_sample * beta be moved out here?

        DBG_PATH_DUMP_PATH_STATE(pathState);

        if (FEATURE_ENABLED(RussianRoulette) && i >= gSettings.RussianRouletteMinBounces)
        {
            float p = saturate(max(pathState.Beta.r, max(pathState.Beta.g, pathState.Beta.b)));
            p = max(p, 0.05f);
            float rRR = Rand01(rngInfo);
            if (rRR > p)
                break;
            pathState.Beta /= p;
        }

        DBG_PATH_DUMP_PATH_STATE(pathState);
    }

    return pathState.Lo;
}

#endif