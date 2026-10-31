#ifndef H_TRACE_SHIFTED_RAY_H
#define H_TRACE_SHIFTED_RAY_H

#include "PathTracing/Shift/HitShiftedUnconnected.hlsli"
#include "PathTracing/Shift/MissShifted.hlsli"
#include "Utils/Debug/Palette.h"

bool ResolveShiftedPathAncillaryVertex(
    inout PathState pathState,
    inout float jacobian,
    inout float pdfRatio,
    inout bool isSymmetric,
    inout ReconnectionState reconnectionState,
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
        isSymmetric = false;
        pathState.Lo = 0.0f;
        DBG_OUTPUT1(1.0f, GD_RejectionEnvMap);
        return false;
    }

    if (isMiss)
    {
        MissShifted(pathState, v1, reconnectionState);
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
        isSymmetric = false;
        pathState.Lo = 0.0f;
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

	float pdfMain = v1.PDF;
	float pdfShifted;

    if (reconnectionState == ReconnectionState::eConnected)
    {
        wi = v1.Wi;

        if (FEAT_CORE(NEE))
        {
            L_sample += v1.DirectContribution * pathState.Beta;
        }

        pathState.Beta *= v1.IndirectContribution;
		pdfShifted = v1.PDF;
    }
    else if (reconnectionState == ReconnectionState::eSemiConnected)
    {
        wi = v1.Wi;

        BxdfEval bxdfEval = BxDF::Evaluate(hitInfo, wo, wi, v1.Lobe);

        if (FEAT_CORE(NEE))
        {
            L_sample += v1.DirectContribution * pathState.Beta;
            pathState.LastBxdfPdf = bxdfEval.PDF; // Note: Not necessary, will be unused once connected. Remove?
        }

        float NdL = dot(hitInfo.Ns_ff, wi);
        pathState.Beta *= bxdfEval.Radiance * abs(NdL) / max(1e-6, v1.PDF);

		pdfShifted = bxdfEval.PDF;

        reconnectionState = ReconnectionState::eConnected;
    }
    else if (reconnectionState == ReconnectionState::eUnconnected)
    {
        if (pathState.RaySegmentIdx == mainVertices.NumVertices - 1) // Note: Only happens due to beta loss deaths
        {
            pathState.Lo = 0.0f;
            isSymmetric = false;
            DBG_OUTPUT1(1.0f, GD_RejectionMaxVertex);
            return false;
        }

        PathVertexInfo v2 = mainVertices.Array[pathState.RaySegmentIdx + 1];
        DBG_OUTPUT3(Palette((uint)v2.Type), GD_V2Type);
        DBG_OUTPUT3(Palette((uint)v2.Lobe), GD_V2Lobe);

        float3 E_indirect;
        HitShiftedUnconnected(reconnectionState, isSymmetric, pathState, v1, v2, hitInfo, wo, wi, E_indirect, jacobian, pdfShifted);

        pathState.Beta *= E_indirect;
    }

    DBG_OUTPUT3(pathState.Beta, GD_BetaLate);

    if (pdfMain <= 0.0f || pdfShifted <= 0.0f)
        isSymmetric = false;

    if (pathState.Beta.x <= 0 && pathState.Beta.y <= 0 && pathState.Beta.z <= 0)
        isSymmetric = false;

    if (!isSymmetric)
    {
        pathState.Lo = 0.0f;
        return false;
    }

	pdfRatio *= (pdfShifted / pdfMain);

    pathState.Lo += ApplyFireflyThreshold(L_sample, gSettings.FireflyThreshold);

    pathState.Desc.Direction = wi;
    pathState.Desc.Origin = hitInfo.HitPos + hitInfo.Ng_ff * EPSILON;;

    DBG_OUTPUT3(wi, GD_L_w);
    DBG_OUTPUT3(hitInfo.SFrame.ToLocal(wi), GD_L_s);
    DBG_OUTPUT1(pdfShifted, GD_PdfShifted);
    DBG_OUTPUT1(0.0f, GD_V2Type);

    pathState.IsPrimaryRay = false;
    return true;
}

// TODO: Put args in ShiftedPathState struct once finalized
bool TraceShiftedPathAncillary(
    inout PathState pathState,
    inout float jacobian,
    inout float pdfRatio,
    inout bool isSymmetric,
    inout ReconnectionState reconnectionState,
    PathVertexList mainVertices,
    int2 pixelCoord)
{
    bool isMiss;
    HitInfo hitInfo;
    ComputeRayHit(pixelCoord, pathState, isMiss, hitInfo);

    return ResolveShiftedPathAncillaryVertex(pathState, jacobian, pdfRatio, isSymmetric, reconnectionState, mainVertices, isMiss, hitInfo);
}

void TraceShiftedPath(
    PathVertexList mainVertices,
    float3 origin,
    float3 dir,
    int2 pixelCoord,

    out float3 Lo,
    out float jacobian,
    inout float pdfRatio,
    out bool isSymmetric)
{
    ReconnectionState reconnectionState = ReconnectionState::eUnconnected;
    isSymmetric = true;
    jacobian = 1.0f;

    PathState pathState = CreatePathState(origin, dir);

    for (uint i = 0; i < mainVertices.NumVertices; i++)
    {
        DBG_SET_CURRENT_RAY_DEPTH(i);
        DBG_PATH_DUMP_MARK_EXPLORED();

        pathState.RaySegmentIdx = i;

        if (!TraceShiftedPathAncillary(pathState, jacobian, pdfRatio, isSymmetric, reconnectionState, mainVertices, pixelCoord))
            break;
    }

    Lo = pathState.Lo;

    DBG_OUTPUT3(Palette((uint)reconnectionState), GD_FinalReconnectionState);
}

#endif