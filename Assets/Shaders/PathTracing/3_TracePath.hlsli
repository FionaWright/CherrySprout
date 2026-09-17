#ifndef H_TRACE_H
#define H_TRACE_H

#include "PathTracing/Structs.h"
#include "PathTracing/Utils.hlsli"
#include "PathTracing/4_Hit.hlsli"
#include "PathTracing/4_Miss.hlsli"
#include "PathTracing/4_ComputeRayHit.hlsli"
#include "PathTracing/Debug/PathDumper.hlsli"

#include "Utils/Random.h"

PathSample TracePath(float3 origin, float3 dir, RngInfo rngInfo, uint2 pixelCoord)
{
    RayQuery<RAY_FLAGS> q;

    PathState pathState = CreatePathState(origin, dir);

    PathVertexList vertexList;
    vertexList.NumVertices = 0;

    uint i;
    for (i = 0; i < gSettings.MaxRayDepth; i++)
    {
        pathState.RaySegmentIdx = i;
        pathState.LastRayWasDiracDelta = false;

        bool isMiss;
        HitInfo hitInfo;
        ComputeRayHit(q, pixelCoord, pathState, isMiss, hitInfo);

        // Debug
        {
            DBG_SET_CURRENT_RAY_DEPTH(i);
            DBG_PATH_DUMP_MARK_EXPLORED();
            DBG_OUTPUT3(Palette(gGBufferMaterialIdx[pixelCoord] - 1),           GBufferMatIdx);
            DBG_OUTPUT3(gGBufferNormals[pixelCoord].rgb,                        GBufferNormalsUnorm);
            DBG_OUTPUT1(gGBufferDepth[pixelCoord].r,                            GBufferDepth);
            DBG_OUTPUT2(gGBufferUvMv[pixelCoord].rg,                            GBufferUv);
            DBG_OUTPUT2(gGBufferUvMv[pixelCoord].ba,                            GBufferMv);
        }

        if (isMiss)
        {
            float3 Li = pathState.Beta * Miss(pathState, i);
            pathState.Lo += ApplyFireflyThreshold(Li);

            DBG_OUTPUT3(Li, MissContrib);
            DBG_OUTPUT1(1.0f, KilledByMiss);

            if (FEATURE_ENABLED(GradientDomain))
            {
                PathVertexInfo currentVertexInfo;
                currentVertexInfo.Type = VertexType::eEnvironment;
                vertexList.Array[vertexList.NumVertices] = currentVertexInfo;
                vertexList.NumVertices++;
                DBG_PATH_DUMP_PATH_VERTEX_INFO(currentVertexInfo);
            }
            break;
        }

        if (FEATURE_ENABLED(Transient))
            pathState.RollingPathDistance += hitInfo.RayT;

        float3 L_sample;
        PathVertexInfo currentVertexInfo;
        Hit(pathState, rngInfo, hitInfo, pixelCoord, L_sample, currentVertexInfo);

        if (FEATURE_ENABLED(GradientDomain))
        {
            vertexList.Array[vertexList.NumVertices] = currentVertexInfo;
            vertexList.NumVertices++;
            DBG_PATH_DUMP_PATH_VERTEX_INFO(currentVertexInfo);
        }

        DBG_PATH_DUMP_PATH_STATE(pathState);

        if (pathState.Beta.x <= 0 && pathState.Beta.y <= 0 && pathState.Beta.z <= 0)
        {
            DBG_OUTPUT1(1.0f, KilledByBetaLoss);
            break;
        }

        pathState.Lo += ApplyFireflyThreshold(L_sample);

        DBG_PATH_DUMP_PATH_STATE(pathState);

        if (FEATURE_ENABLED(RussianRoulette) && i >= gSettings.RussianRouletteMinBounces)
        {
            float p = saturate(max(pathState.Beta.r, max(pathState.Beta.g, pathState.Beta.b)));
            p = max(p, 0.05f);
            float rRR = Rand01(rngInfo);
            if (rRR > p)
            {
                DBG_OUTPUT1(1.0f, KilledByRR);
                break;
            }
            pathState.Beta /= p;
        }

        DBG_PATH_DUMP_PATH_STATE(pathState);
    }

    DBG_OUTPUT1(i == gSettings.MaxRayDepth, KilledByMaxDepth);
    DBG_OUTPUT1(0.0f, KilledByBetaLoss);
    DBG_OUTPUT1(0.0f, KilledByRR);
    DBG_OUTPUT1(0.0f, KilledByMiss);

    PathSample pathSample;
    pathSample.Lo = pathState.Lo;

    if (FEATURE_ENABLED(GradientDomain))
        pathSample.VertexList = vertexList;

    return pathSample;
}

#endif