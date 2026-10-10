#ifndef H_TRACE_SHIFTED_RAY_H
#define H_TRACE_SHIFTED_RAY_H

#include "PathTracing/GradientDomain/HitShiftedUnconnected.hlsli"
#include "Utils/Debug/Palette.h"

// TODO: IsPrimaryRay field
// TODO: NEE updated

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
    RayQuery<RAY_FLAGS> q;

    ReconnectionState reconnectionState = ReconnectionState::eUnconnected;
    isSymmetric = true;
    jacobian = 1.0f;

    PathState pathState = CreatePathState(origin, dir);

    for (uint i = 0; i < mainVertices.NumVertices; i++)
    {
        PathVertexInfo v1 = mainVertices.Array[i];

        DBG_SET_CURRENT_RAY_DEPTH(i);
        DBG_PATH_DUMP_MARK_EXPLORED();
        DBG_OUTPUT3(Palette((uint)v1.Type), GD_V1Type);

        pathState.RaySegmentIdx = i;
        //pathState.LastRayWasDiracDelta = false;

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

        // TODO: EvaluateEmission
        float3 L_sample = pathState.Beta * hitInfo.Emission; // TODO: Emission can be cached for connected rays

        float3 wo = -pathState.Desc.Direction;
        float3 wi;

        DBG_OUTPUT3(wo, GD_V_w);
        DBG_OUTPUT3(hitInfo.SFrame.ToLocal(wo), GD_V_s);
        DBG_OUTPUT3(pathState.Beta, GD_BetaEarly);
        DBG_OUTPUT3(hitInfo.Emission, GD_Emission);

        float3 nextOrigin = hitInfo.HitPos + hitInfo.Ng_ff * EPSILON;

		float pdfMain = v1.PDF;
		float pdfShifted;

        if (reconnectionState == ReconnectionState::eConnected)
        {
            wi = v1.Wi;

            if (FEAT_CORE(NEE))
            {
                L_sample += v1.DirectContribution * pathState.Beta;
                pathState.LastBxdfPdf = v1.PDF_Bxdf;
            }

            pathState.Beta *= v1.IndirectContribution;
			pdfShifted = v1.PDF;
        }
        else if (reconnectionState == ReconnectionState::eSemiConnected)
        {
            wi = v1.Wi;

            BxDF bxdf;
            float3 f_bxdf;
            float pdf_bxdf;
            bxdf.Evaluate(hitInfo, wo, wi, f_bxdf, pdf_bxdf);

            if (FEAT_CORE(NEE))
            {
                L_sample += v1.DirectContribution * pathState.Beta;
                pathState.LastBxdfPdf = pdf_bxdf;
            }

            float NdL = dot(hitInfo.Ns_ff, wi);
            pathState.Beta *= f_bxdf * abs(NdL) / max(1e-6, pdf_bxdf);

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

            float3 E_indirect;
            HitShiftedUnconnected(reconnectionState, isSymmetric, pathState, v1, v2, hitInfo, wo, nextOrigin, wi, E_indirect, jacobian, pdfShifted);

            pathState.Beta *= E_indirect;
        }

        DBG_OUTPUT3(pathState.Beta, GD_BetaLate);

        if (pdfMain <= 0.0f || pdfShifted <= 0.0f)
            isSymmetric = false;

        if (pathState.Beta.x <= 0 && pathState.Beta.y <= 0 && pathState.Beta.z <= 0)
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