#ifndef H_TRACE_SHIFTED_RAY_H
#define H_TRACE_SHIFTED_RAY_H

#include "PathTracing/Shift/SampleDirect_Shifted.hlsli"
#include "PathTracing/Shift/SampleIndirect_Shifted.hlsli"
#include "PathTracing/Shift/Miss_Shifted.hlsli"
#include "PathTracing/EvaluateHitEmission.hlsli"
#include "PathTracing/4_ComputeRayHit.hlsli"
#include "Utils/Debug/Palette.h"

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
        Miss_S(pathState, v1, pathStateShifted.ReconnectionState);
        return false;
    }

    float3 wo = -pathState.Desc.Direction;

    float mis_emission;
    float3 L_emission = EvaluateHitEmission(hitInfo, pathState, wo, mis_emission);
    if (v1.Lobe != LobeTag::eNEE)
        L_emission *= mis_emission;

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

    if (isEmissive) // TODO: When connected, reuse v1 emission
    {
        float3 L_sample = pathState.Beta * L_emission;
        pathState.Lo += ApplyFireflyThreshold(L_sample, gSettings.FireflyThreshold);
        return false;
    }

    if (FEAT_CORE(NEE) && !pathState.GetPathFlag(eIsTerminalRay))
    {
        if (!SampleDirect_S(pathState, pathStateShifted, v1, hitInfo, wo))
            return false;
    }

    DBG_OUTPUT1(0.0f, GD_Nee_MainLo);
    DBG_OUTPUT1(0.0f, GD_Nee_ShiftLo);
    DBG_OUTPUT1(0.0f, GD_Nee_Gradient);
    DBG_OUTPUT1(0.0f, GD_Nee_MIS);
    DBG_OUTPUT1(1.0f, GD_Nee_Jacobian);

    if (pathState.GetPathFlag(eIsTerminalRay))
    {
        if (!FEAT_CORE(NEE))
        {
            pathStateShifted.IsSymmetric = false;
            DBG_OUTPUT1(1.0f, GD_RejectionMaxVertex);
        }
        return false;
    }

    float3 wi;
    float pdfShifted;
    if (!SampleIndirect_S(pathState, pathStateShifted, mainVertices, v1, hitInfo, wo, wi, pdfShifted))
        return false;

    DBG_OUTPUT3(pathState.Beta, GD_BetaLate);

    float pdfMain = v1.PDF_BXDF;

    pathStateShifted.IsSymmetric &= pdfMain > 0.0f;
    pathStateShifted.IsSymmetric &= pdfShifted > 0.0f;
    pathStateShifted.IsSymmetric &= any(pathState.Beta > 0.0f);
    if (!pathStateShifted.IsSymmetric)
        return false;

	pathStateShifted.LogPathPdfMain += log(pdfMain);
	pathStateShifted.LogPathPdfShifted += log(pdfShifted);
	pathStateShifted.PdfRatio *= (pdfShifted / pdfMain);

    pathState.Desc.Direction = wi;
    pathState.Desc.Origin = hitInfo.HitPos + hitInfo.Ng_ff * EPSILON;;

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
    out float3 subpathGradients,
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
        pathState.SetPathFlag(eIsPrimaryRay, i == 0);
        pathState.SetPathFlag(eIsTerminalRay, i == mainVertices.NumVertices - 1);

        if (!TraceShiftedPathAncillary(pathState, pathStateShifted, mainVertices, pixelCoord))
            break;
    }

    Lo = pathState.Lo;
    jacobian = pathStateShifted.Jacobian;
    pdfRatio = pathStateShifted.PdfRatio;
    isSymmetric = pathStateShifted.IsSymmetric;
    subpathGradients = pathStateShifted.SubpathGradients;

    float3 dbgRcStateColor =
        pathStateShifted.ReconnectionState == ReconnectionState::eUnconnected ? float3(1,0,0) :
        pathStateShifted.ReconnectionState == ReconnectionState::eSemiConnected ? float3(0.5,0.5,0) : float3(0,1,0);
    DBG_OUTPUT3(dbgRcStateColor, GD_FinalReconnectionState);
}

#endif