#ifndef H_TRACE_SHIFTED_RAY_H
#define H_TRACE_SHIFTED_RAY_H

#include "PathTracing/ShiftMapping/ShiftMapping.hlsli"
#include "Utils/Debug/Palette.h"

enum class ReconnectionState : uint
{
    eUnconnected,
    eSemiConnected,  // Connected but has different wo so needs BxDF evals
    eConnected,      // Can reuse Hit contribution
};

void HitShiftedUnconnected(
    inout ReconnectionState reconnectionState,
    inout bool isSymmetric,
    inout PathState pathState,

    PathVertexInfo v1,
    PathVertexInfo v2,
    HitInfo hitInfo,
    float3 wo,
    float3 nextOrigin,

    out float3 wi,
	out float pdf)
{
    if (v2.Type == VertexType::eUninitialized) // (Path terminates on v1)
    {
        isSymmetric = false;
        wi = NAN;
        pdf = NAN;
        DBG_OUTPUT1(1.0f, GD_RejectionUninit);
        return;
    }

    VertexType shiftedType = GetIsVertexDiffuse(hitInfo.Mat.Roughness) ? VertexType::eDiffuse : VertexType::eGlossy;
    if (v1.Type != shiftedType)
    {
        isSymmetric = false;
        wi = NAN;
        pdf = NAN;
        DBG_OUTPUT1(1.0f, GD_RejectionVertexMismatch);
        return;
    }

    ShiftResult shiftResult;

    if (v1.Type == VertexType::eDiffuse)
    {
        if (v2.Type == VertexType::eDiffuse)
        {
            shiftResult = ShiftReconnect(v1.Position, nextOrigin, hitInfo.Ns_ff, v2.Position, v2.SFrame.N);
        }
        else if (v2.Type == VertexType::eEnvironment)
        {
            shiftResult = ShiftReconnectEnvironment(nextOrigin, hitInfo.Ns_ff, v1.Wi);
        }
        else if (v2.Type == VertexType::eGlossy)
        {
            // TODO
        }

        reconnectionState = ReconnectionState::eSemiConnected;
    }
    else if (v1.Type == VertexType::eGlossy)
    {
        float iorNCurrent =  hitInfo.IsEntering ? IOR_N_AIR          : hitInfo.Mat.IOR_N;
        float iorNNext =     hitInfo.IsEntering ? hitInfo.Mat.IOR_N  : IOR_N_AIR;
        float eta = iorNCurrent / iorNNext;

        // Half-Vector shift can also be achieved by copying rngInfo and sampling BxDF, but this seems more complex
        shiftResult = ShiftHalfVector(v1.SFrame, hitInfo.SFrame, v1.Wi, v1.Wo, wo, v1.Eta, eta);
    }
    else
        DBG_ASSERT_FAIL(GD_INVALID_VERTEX_TYPE);

    if (!shiftResult.IsSuccessful)
    {
        isSymmetric = false;
        wi = NAN;
        pdf = NAN;
        DBG_OUTPUT1(1.0f, GD_RejectionShiftMapping);
        return;
    }

    wi = shiftResult.Wi;

    BxDF bxdf;
    float3 f_bxdf;
    float pdf_bxdf;
    bxdf.Evaluate(hitInfo, wo, wi, f_bxdf, pdf_bxdf);

    float NdL = dot(hitInfo.Ns_ff, wi);
    pathState.Beta *= f_bxdf * abs(NdL) / max(1e-6, pdf_bxdf);

    if (FEATURE_ENABLED(NEE))
        pathState.LastBxdfPdf = pdf_bxdf;

	pdf = shiftResult.Jacobian * pdf_bxdf;

    DBG_OUTPUT1(shiftResult.Jacobian, GD_Jacobian);

    // TODO: Direct lighting
}

void TraceShiftedPath(
    PathVertexList mainVertices,
    float3 origin,
    float3 dir,
    int2 pixelCoord,

    out float3 Lo,
    inout float pdfRatio,
    out bool isSymmetric)
{
    RayQuery<RAY_FLAGS> q;

    ReconnectionState reconnectionState = ReconnectionState::eUnconnected;
    isSymmetric = true;

    PathState pathState = CreatePathState(origin, dir);

    for (uint i = 0; i < mainVertices.NumVertices; i++)
    {
        PathVertexInfo v1 = mainVertices.Array[i];

        DBG_SET_CURRENT_RAY_DEPTH(i);
        DBG_PATH_DUMP_MARK_EXPLORED();
        DBG_OUTPUT3(Palette((uint)v1.Type), GD_V1Type);

        pathState.RaySegmentIdx = i;
        pathState.LastRayWasDiracDelta = false;

        // TODO: Skip tracing once connected
        bool isMiss;
        HitInfo hitInfo;
        ComputeRayHit(q, pixelCoord, pathState, isMiss, hitInfo);

        bool mainIsMiss = v1.Type == VertexType::eEnvironment;
        if (isMiss != mainIsMiss)
        {
            isSymmetric = false;
            Lo = 0.0f;
            DBG_OUTPUT1(1.0f, GD_RejectionEnvMap);
            break;
        }

        if (isMiss)
        {
            // TODO: Value can be cached if connected (likely)
            float3 Li = pathState.Beta * Miss(pathState, i);
            pathState.Lo += ApplyFireflyThreshold(Li, gSettings.FireflyThreshold);
            break;
        }

        float3 L_sample = pathState.Beta * hitInfo.Emission; // TODO: Emission can be cached for connected rays

        float3 wo = -pathState.Desc.Direction;
        float3 wi;

        DBG_OUTPUT3(wo, GD_V_w);
        DBG_OUTPUT3(hitInfo.SFrame.ToLocal(wo), GD_V_s);
        DBG_OUTPUT3(pathState.Beta, GD_BetaEarly);
        DBG_OUTPUT3(hitInfo.Emission, GD_Emission);

        float3 hitPos = pathState.Desc.Origin + pathState.Desc.Direction * hitInfo.RayT;
        float3 nextOrigin = hitPos + hitInfo.Ng_ff * EPSILON;

		float pdfMain = v1.PDF;
		float pdfShifted;

        if (reconnectionState == ReconnectionState::eConnected)
        {
            wi = v1.Wi;
            pathState.Beta *= v1.IndirectContribution;
			pdfShifted = v1.PDF; // Note: PDFs match, ratio unchanged

            if (FEATURE_ENABLED(NEE))
                pathState.LastBxdfPdf = NAN;
        }
        else if (reconnectionState == ReconnectionState::eSemiConnected)
        {
            wi = v1.Wi;

            BxDF bxdf;
            float3 f_bxdf;
            float pdf_bxdf;
            bxdf.Evaluate(hitInfo, wo, wi, f_bxdf, pdf_bxdf);

            float NdL = dot(hitInfo.Ns_ff, wi);
            pathState.Beta *= f_bxdf * abs(NdL) / max(1e-6, pdf_bxdf);

            if (FEATURE_ENABLED(NEE))
                pathState.LastBxdfPdf = pdf_bxdf;

			pdfShifted = pdf_bxdf;

            reconnectionState = ReconnectionState::eConnected;
        }
        else if (reconnectionState == ReconnectionState::eUnconnected)
        {
            if (i == mainVertices.NumVertices - 1) // Note: Only really happens due to beta loss deaths
            {
                Lo = 0.0f;
                isSymmetric = false;
                DBG_OUTPUT1(1.0f, GD_RejectionMaxVertex);
                break;
            }

            PathVertexInfo v2 = mainVertices.Array[i+1];
            DBG_OUTPUT3(Palette((uint)v2.Type), GD_V2Type);

            HitShiftedUnconnected(reconnectionState, isSymmetric, pathState, v1, v2, hitInfo, wo, nextOrigin, wi, pdfShifted);
        }

        DBG_OUTPUT3(pathState.Beta, GD_BetaLate);

        if (pdfMain <= 0.0f || pdfShifted <= 0.0f)
            isSymmetric = false;

        if (!isSymmetric)
        {
            Lo = 0.0f;
            break;
        }

		pdfRatio *= (pdfShifted / pdfMain);

        pathState.Lo += ApplyFireflyThreshold(L_sample, gSettings.FireflyThreshold);

        pathState.Desc.Direction = wi;
        pathState.Desc.Origin = nextOrigin;

        DBG_OUTPUT3(wi, GD_L_w);
        DBG_OUTPUT3(hitInfo.SFrame.ToLocal(wi), GD_L_s);
        DBG_OUTPUT1(pdfShifted, GD_PdfShifted);
        DBG_OUTPUT1(0.0f, GD_V2Type);
    }

    Lo = pathState.Lo;

    DBG_OUTPUT3(Palette((uint)reconnectionState), GD_FinalReconnectionState);
}

#endif