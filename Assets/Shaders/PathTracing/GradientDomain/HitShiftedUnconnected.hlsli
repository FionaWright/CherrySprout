#ifndef H_HIT_SHIFTED_UNCONNECTED_H
#define H_HIT_SHIFTED_UNCONNECTED_H

#include "PathTracing/ShiftMapping/ShiftMapping.hlsli"

void HitShiftedUnconnected(
    inout ReconnectionState reconnectionState,
    inout bool isSymmetric,
    inout PathState pathState,

    PathVertexInfo v1,
    PathVertexInfo v2,
    HitInfo hitInfo,
    float3 wo,
    float3 hitPos,
    float3 nextOrigin,

    out float3 wi,
    out float3 contrib,
    inout float jacobian,
	out float pdf)
{
    pdf = 1.0f;

    VertexType shiftedType = GetIsVertexDiffuse(hitInfo.Mat.Roughness) ? VertexType::eDiffuse : VertexType::eGlossy;
    if (v1.Type != shiftedType || v2.Type == VertexType::eUninitialized || v2.Type == VertexType::eGlossy)
    {
        isSymmetric = false;
        contrib = 1.0f;
        wi = NAN;
        DBG_OUTPUT1(1.0f, GD_RejectionVertexMismatch);
        return;
    }

    ShiftResult shiftResult;

    if (v1.Type == VertexType::eDiffuse)
    {
        if (v2.Type == VertexType::eDiffuse)
            shiftResult = ShiftReconnect(v1.Position, nextOrigin, hitInfo.Ns_ff, v2.Position, v2.SFrame.N);
        else if (v2.Type == VertexType::eEnvironment)
            shiftResult = ShiftReconnectEnvironment(nextOrigin, hitInfo.Ns_ff, v1.Wi);

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
        contrib = 1.0f;
        wi = NAN;
        DBG_OUTPUT1(1.0f, GD_RejectionShiftMapping);
        return;
    }

    wi = shiftResult.Wi;
    jacobian *= shiftResult.Jacobian;
    DBG_OUTPUT1(shiftResult.Jacobian, GD_Jacobian);

    float NdL = dot(hitInfo.Ns_ff, wi);

    BxDF bxdf;
    float3 f_bxdf;
    float pdf_bxdf;
    bxdf.Evaluate(hitInfo, wo, wi, f_bxdf, pdf_bxdf);

    contrib = f_bxdf * abs(NdL) / max(1e-6, pdf_bxdf);

    pdf *= pdf_bxdf;

    if (FEATURE_ENABLED(NEE))
    {
        pathState.LastBxdfPdf = pdf_bxdf;

        // TODO: Multiple NEE samples

        float3 E_direct = 0;
        //for (uint i = 0; i < gSettings.DirectNumSamples; i++)
        //{
        //    E_direct += EvaluateDirectLighting(hitInfo, pathState, bxdf, pixelCoord, wo, hitPos, nextOrigin);
        //}

        uint lightIdx = v1.NeeLightIdx;
        float3 lightDir = v1.NeeLightDirection;

        float pdf_nee;
        E_direct = EvaluateNEE(hitInfo, pathState, bxdf, 1, wo, hitPos, nextOrigin, lightIdx, lightDir, pdf_nee);

        pdf *= pdf_nee;

        contrib += E_direct * pathState.Beta;
        //contrib += E_direct * pathState.Beta / float(gSettings.DirectNumSamples);
    }
}

#endif