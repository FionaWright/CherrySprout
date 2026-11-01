#ifndef H_TRACE_SHIFTED_RAY_H
#define H_TRACE_SHIFTED_RAY_H

#include "PathTracing/Shift/HitShiftedUnconnected.hlsli"
#include "PathTracing/Shift/MissShifted.hlsli"
#include "PathTracing/EvaluateHitEmission.hlsli"
#include "PathTracing/4_ComputeRayHit.hlsli"
#include "Utils/Debug/Palette.h"

void ResolveConnected(inout PathState pathState, inout float3 L_sample, PathVertexInfo v1, out float3 wi, out float pdf)
{
    wi = v1.Wi;
    pdf = v1.PDF;

    if (FEAT_CORE(NEE))
        L_sample += v1.DirectContribution * pathState.Beta;

    pathState.Beta *= v1.IndirectContribution;
}

void ResolveSemiConnected(inout PathState pathState, inout float3 L_sample, PathVertexInfo v1, HitInfo hitInfo, float3 wo, out float3 wi, out float pdf)
{
    wi = v1.Wi;

    BxdfEval bxdfEval = BxDF::Evaluate(hitInfo, wo, wi, v1.Lobe);

    pdf = bxdfEval.PDF;

    if (FEAT_CORE(NEE))
    {
        L_sample += v1.DirectContribution * pathState.Beta;
        pathState.LastBxdfPdf = bxdfEval.PDF; // Note: Not necessary, will be unused once connected. Remove?
    }

    float NdL = dot(hitInfo.Ns_ff, wi);
    pathState.Beta *= bxdfEval.Radiance * abs(NdL) / max(1e-6f, v1.PDF);
}

bool ResolveShiftedPathAncillaryVertex(
    inout PathState pathState,
    inout PathStateShifted pathStateShifted,
    PathVertexList mainVertices,
    bool isMiss,
    HitInfo hitInfo)
{
    PathVertexInfo v1 = mainVertices.Array[pathState.RaySegmentIdx];

    DBG_OUTPUT3(Palette((uint)v1.Type), GD_V1Type);
    DBG_OUTPUT3(Palette((uint)v1.Lobe), GD_V1Lobe);

    bool mainIsMiss = v1.Type == VertexType::eEnvironment;
    if (isMiss != mainIsMiss)
    {
        pathStateShifted.IsSymmetric = false;
        DBG_OUTPUT1(1.0f, GD_RejectionEnvMap);
        return false;
    }

    if (isMiss)
    {
        MissShifted(pathState, v1, pathStateShifted.ReconnectionState);
        return false;
    }

    float3 L_sample = 0.0f;

    float3 wo = -pathState.Desc.Direction;

    float3 L_emission = EvaluateHitEmission(hitInfo, pathState, wo);

    DBG_OUTPUT3(L_emission, GD_Emission);
    DBG_OUTPUT3(wo, GD_V_w);
    DBG_OUTPUT3(hitInfo.SFrame.ToLocal(wo), GD_V_s);
    DBG_OUTPUT3(pathState.Beta, GD_BetaEarly);

    bool isEmissive = any(L_emission > 0.0f);
    bool mainIsEmissive = v1.Type == VertexType::eEmission;
    if (isEmissive != mainIsEmissive)
    {
        pathStateShifted.IsSymmetric = false;
        DBG_OUTPUT1(1.0f, GD_RejectionEmission);
        return false;
    }

    if (isEmissive)
    {
        L_sample += pathState.Beta * L_emission;
        pathState.Lo += ApplyFireflyThreshold(L_sample, gSettings.FireflyThreshold);
        return false;
    }

    float3 wi;
	float pdfShifted;

    if (pathStateShifted.ReconnectionState == ReconnectionState::eConnected)
    {
        ResolveConnected(pathState, L_sample, v1, wi, pdfShifted);
    }
    else if (pathStateShifted.ReconnectionState == ReconnectionState::eSemiConnected)
    {
        ResolveSemiConnected(pathState, L_sample, v1, hitInfo, wo, wi, pdfShifted);
        pathStateShifted.ReconnectionState = ReconnectionState::eConnected;
    }
    else if (pathStateShifted.ReconnectionState == ReconnectionState::eUnconnected)
    {
        bool mainPathBetaLoss = pathState.RaySegmentIdx == mainVertices.NumVertices - 1;
        if (mainPathBetaLoss) // TODO: Allow symmetric?
        {
            pathStateShifted.IsSymmetric = false;
            DBG_OUTPUT1(1.0f, GD_RejectionMaxVertex);
            return false;
        }

        PathVertexInfo v2 = mainVertices.Array[pathState.RaySegmentIdx + 1];

        float3 E_indirect;
        HitShiftedUnconnected(pathState, pathStateShifted, v1, v2, hitInfo, wo, wi, E_indirect, pdfShifted);

        pathState.Beta *= E_indirect;

        DBG_OUTPUT3(Palette((uint)v2.Type), GD_V2Type);
        DBG_OUTPUT3(Palette((uint)v2.Lobe), GD_V2Lobe);
    }

    DBG_OUTPUT3(pathState.Beta, GD_BetaLate);

    float pdfMain = v1.PDF;

    pathStateShifted.IsSymmetric &= pdfMain > 0.0f;
    pathStateShifted.IsSymmetric &= pdfShifted > 0.0f;
    pathStateShifted.IsSymmetric &= any(pathState.Beta > 0.0f);
    if (!pathStateShifted.IsSymmetric)
        return false;

	pathStateShifted.PdfRatio *= (pdfShifted / pdfMain);

    pathState.Lo += ApplyFireflyThreshold(L_sample, gSettings.FireflyThreshold);
    pathState.Desc.Direction = wi;
    pathState.Desc.Origin = hitInfo.HitPos + hitInfo.Ng_ff * EPSILON;;
    pathState.IsPrimaryRay = false;

    DBG_OUTPUT3(wi, GD_L_w);
    DBG_OUTPUT3(hitInfo.SFrame.ToLocal(wi), GD_L_s);
    DBG_OUTPUT1(pdfShifted, GD_PdfShifted);
    DBG_OUTPUT1(0.0f, GD_V2Type);
    return true;
}

bool TraceShiftedPathAncillary(
    inout PathState pathState,
    inout PathStateShifted pathStateShifted,
    PathVertexList mainVertices,
    int2 pixelCoord)
{
    bool isMiss;
    HitInfo hitInfo;
    ComputeRayHit(pixelCoord, pathState, isMiss, hitInfo);

    return ResolveShiftedPathAncillaryVertex(pathState, pathStateShifted, mainVertices, isMiss, hitInfo);
}

void TraceShiftedPath(
    PathVertexList mainVertices,
    float3 origin,
    float3 dir,
    int2 pixelCoord,

    out float3 Lo,
    out float jacobian,
    out float pdfRatio,
    out bool isSymmetric)
{
    PathState pathState = CreatePathState(origin, dir);
    PathStateShifted pathStateShifted = CreatePathStateShifted();

    DBG_ASSERT_LE(mainVertices.NumVertices, gSettings.MaxRayDepth, MAX_RAY_DEPTH_SHIFT_ID);

    for (uint i = 0; i < mainVertices.NumVertices; i++)
    {
        DBG_SET_CURRENT_RAY_DEPTH(i);
        DBG_PATH_DUMP_MARK_EXPLORED();

        pathState.RaySegmentIdx = i;

        if (!TraceShiftedPathAncillary(pathState, pathStateShifted, mainVertices, pixelCoord))
            break;
    }

    Lo = pathStateShifted.IsSymmetric ? pathState.Lo : 0.0f;
    jacobian = pathStateShifted.Jacobian;
    pdfRatio = pathStateShifted.PdfRatio;
    isSymmetric = pathStateShifted.IsSymmetric;

    DBG_OUTPUT3(Palette((uint)pathStateShifted.ReconnectionState), GD_FinalReconnectionState);
}

#endif