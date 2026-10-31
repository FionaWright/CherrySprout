#ifndef H_TRACE_H
#define H_TRACE_H

#include "PathTracing/Structs.h"
#include "PathTracing/Utils.hlsli"
#include "PathTracing/4_Hit.hlsli"
#include "PathTracing/4_Miss.hlsli"
#include "PathTracing/4_ComputeRayHit.hlsli"
#include "PathTracing/Debug/PathDumper.hlsli"
#include "PathTracing/Debug/Globals.hlsli"
#include "PathTracing/ReSTIR/GI.hlsli"
#include "PathTracing/GradientDomain/Utils.hlsli"

#include "PathTracing/RandomSample.hlsli"
#include "Utils/HlslUtils.hlsli"

bool TracePathAncillary(inout PathState pathState, inout PathVertexList vertexList, inout RngInfo rngInfo, uint2 pixelCoord)
{
    RayQuery<RAY_FLAGS> q;

    bool isMiss;
    HitInfo hitInfo;
    ComputeRayHit(q, pixelCoord, pathState, isMiss, hitInfo);

    // Debug
    {
        DBG_SET_CURRENT_RAY_DEPTH(pathState.RaySegmentIdx);
        DBG_PATH_DUMP_MARK_EXPLORED();
        DBG_PATH_DUMP_PATH_STATE(pathState);
        DBG_OUTPUT3(Palette(gGBufferMaterialIdx[pixelCoord] - 1),             GBuffer_MatIdx);
        DBG_OUTPUT3(DecodeNormalOctahedral(gGBufferNormalsUv[pixelCoord].xy), GBuffer_NormalsUnorm);
        DBG_OUTPUT1(gGBufferDepth[pixelCoord].r,                              GBuffer_Depth);
        DBG_OUTPUT2(gGBufferNormalsUv[pixelCoord].zw,                         GBuffer_Uv);
        DBG_OUTPUT2(gGBufferMv[pixelCoord].xy,                                GBuffer_Mv);
    }

    if (isMiss)
    {
        float3 L_miss = Miss(pathState);
        float3 L_direct = pathState.Beta * L_miss;
        pathState.Lo += ApplyFireflyThreshold(L_direct, gSettings.FireflyThreshold);

        DBG_OUTPUT3(L_direct, MissContrib);
        DBG_OUTPUT1(1.0f, KilledByMiss);

        if (FEAT_CORE(GradientDomain))
            AddVertexEnvMap(vertexList, L_miss);

        return false;
    }

    float3 wo = -pathState.Desc.Direction;
    float3 L_emission = EvaluateHitEmission(hitInfo, pathState, wo);
    bool isEmissive = any(L_emission > 0.0f);
    if (isEmissive)
    {
        float3 L_sample = pathState.Beta * L_emission;
        pathState.Lo += ApplyFireflyThreshold(L_sample, gSettings.FireflyThreshold);

        DBG_OUTPUT1(1.0f, KilledByEmission);

        if (FEAT_CORE(GradientDomain))
            AddVertexEmissive(vertexList, hitInfo.HitPos, hitInfo.Ng_ff, hitInfo.Mat.Roughness);

        return false;
    }

    RngValuesAncillary rng = INIT0(RngValuesAncillary);
    SampleRngAncillaryBatch1(rngInfo, rng);

    if (FEAT_CORE(AlphaTesting))
    {
        float alpha = hitInfo.Mat.Albedo.a;
        bool cutout = alpha < EPSILON || rng.AlphaTest > alpha;
        if (cutout)
        {
            float NdV = dot(hitInfo.Ng_ff, pathState.Desc.Direction);
            pathState.Desc.Origin = hitInfo.HitPos + hitInfo.Ng_ff * EPSILON * sign(NdV);
            return true;
        }
    }

    SampleRngAncillaryBatch2(rngInfo, rng);

    if (FEAT_CORE(Transient))
        pathState.RollingPathDistance += hitInfo.RayT;

    if (FEAT_CORE(RestirGI) && pathState.IsPrimaryRay)
    {
        CandidateGI Y;
        float3 L_gi;
        TrySampleReservoirGI(hitInfo, pathState.Beta, pixelCoord, wo, Y, L_gi);
        pathState.Lo += ApplyFireflyThreshold(L_gi, gSettings.FireflyThreshold);

        /*
            TODO:
            Consider how this contribution should fit into the path-tracer
            Do I need MIS? Or randomly select one estimator? Or fixed weights (0.5,0.5)?
            How does it interact with ReSTIR DI? Is that another estimator to keep track of?
        */

        DBG_OUTPUT3(L_gi, RESTIRGI_GiLo);
    }

    float3 L_sample;
    PathVertexInfo vInfo = INIT0(PathVertexInfo);
    vInfo.PDF = 1.0f;
    Hit(pathState, rng, hitInfo, pixelCoord, L_sample, vInfo);

    if (FEAT_CORE(GradientDomain))
        AddVertex(vertexList, vInfo); // TODO: RNG Ancillary batch 3 not included, maybe never matters

    pathState.Lo += ApplyFireflyThreshold(L_sample, gSettings.FireflyThreshold);

    DBG_PATH_DUMP_PATH_STATE(pathState);

    if (pathState.Beta.x <= 0 && pathState.Beta.y <= 0 && pathState.Beta.z <= 0)
    {
        DBG_OUTPUT1(1.0f, KilledByBetaLoss);
        return false;
    }

    SampleRngAncillaryBatch3(rngInfo, rng);

    if (FEAT_CORE(RussianRoulette) && pathState.RaySegmentIdx >= gSettings.RussianRouletteMinBounces)
    {
        float p = saturate(max(pathState.Beta.r, max(pathState.Beta.g, pathState.Beta.b)));
        p = max(p, 0.05f);
        if (rng.RussianRoulette > p)
        {
            DBG_OUTPUT1(1.0f, KilledByRR);
            return false;
        }
        pathState.Beta /= p;
    }

    DBG_PATH_DUMP_PATH_STATE(pathState);

    pathState.IsPrimaryRay = false;
    return true;
}

PathSample TracePath(float3 origin, float3 dir, RngInfo rngInfo, uint2 pixelCoord)
{
    PathState pathState = CreatePathState(origin, dir);

    PathVertexList vertexList = INIT0(PathVertexList);
    vertexList.NumVertices = 0;

    for (uint i = 0; i < gSettings.MaxRayDepth; i++)
    {
        pathState.RaySegmentIdx = i;

        if (!TracePathAncillary(pathState, vertexList, rngInfo, pixelCoord))
            break;
    }

    PathSample pathSample = INIT0(PathSample);
    pathSample.Lo = pathState.Lo;

    if (FEAT_CORE(GradientDomain))
        pathSample.VertexList = vertexList;

    DBG_OUTPUT1(pathState.RaySegmentIdx == gSettings.MaxRayDepth, KilledByMaxDepth);
    DBG_OUTPUT_DUMP_DISABLE();
    {
        DBG_OUTPUT1(0.0f, KilledByBetaLoss);
        DBG_OUTPUT1(0.0f, KilledByRR);
        DBG_OUTPUT1(0.0f, KilledByMiss);
        DBG_OUTPUT1(0.0f, KilledByEmission);
        DBG_OUTPUT1(0.0f, RESTIR_ReservoirIdx);
        DBG_OUTPUT1(0.0f, RESTIR_ReservoirWeightSum);
        DBG_OUTPUT1(0.0f, RESTIR_ReservoirConfidence);
        DBG_OUTPUT1(0.0f, RESTIR_NdL);
        DBG_OUTPUT1(0.0f, RESTIR_W_Y);
        DBG_OUTPUT1(0.0f, RESTIRGI_Dir);
        DBG_OUTPUT1(0.0f, RESTIRGI_DirectContrib);
        DBG_OUTPUT1(0.0f, RESTIRGI_BxdfRadiance);
        DBG_OUTPUT1(0.0f, RESTIRGI_BxdfPDF);
        DBG_OUTPUT1(0.0f, RESTIRGI_Jacobian);
        DBG_OUTPUT1(0.0f, RESTIRGI_GiLo);
    }
    DBG_OUTPUT_DUMP_ENABLE();

    return pathSample;
}

#endif