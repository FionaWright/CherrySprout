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

    out float3 wi,
    out float3 contrib,
    inout float jacobian,
	out float pdf)
{
    pdf = 1.0f;

    if (IsGlossy(v2.Lobe))
    {
        isSymmetric = false;
        contrib = 1.0f;
        wi = NAN;
        DBG_OUTPUT1(1.0f, GD_RejectionUninit);
        DBG_OUTPUT1(1.0f, GD_RejectionVertexMismatch); // TODO: Remove
        return;
    }

    ShiftResult shiftResult = (ShiftResult)0;

    if (IsGlossy(v1.Lobe))
    {
        float iorNCurrent =  hitInfo.IsEntering ? IOR_N_AIR          : hitInfo.Mat.IOR_N;
        float iorNNext =     hitInfo.IsEntering ? hitInfo.Mat.IOR_N  : IOR_N_AIR;
        float eta = iorNCurrent / iorNNext;

        shiftResult = ShiftHalfVector(v1.SFrame, hitInfo.SFrame, v1.Wi, v1.Wo, wo, v1.Eta, eta);
    }
    else
    {
        if (v2.Type == VertexType::eStandard || v2.Type == VertexType::eEmission)
            shiftResult = ShiftReconnectVis(v1.Position, hitInfo.HitPosBiased, hitInfo.Ng_ff, v2.Position, v2.Ng_ff);
        else if (v2.Type == VertexType::eEnvironment)
            shiftResult = ShiftReconnectEnvironmentVis(hitInfo.HitPosBiased, hitInfo.Ng_ff, v1.Wi);
		else
        	DBG_ASSERT_FAIL(GD_INVALID_VERTEX_TYPE);

        reconnectionState = ReconnectionState::eSemiConnected;
    }

    if (!shiftResult.IsSuccessful)
    {
        isSymmetric = false;
        contrib = 0.0f;
        wi = NAN;
        DBG_OUTPUT1(1.0f, GD_RejectionShiftMapping);
        return;
    }

    wi = shiftResult.Wi;
    jacobian *= shiftResult.Jacobian;
    DBG_OUTPUT1(shiftResult.Jacobian, GD_Jacobian);

    float NdL = dot(hitInfo.Ns_ff, wi);

    BxDF bxdf;
    BxdfEval bxdfEval = bxdf.Evaluate(hitInfo, wo, wi, v1.Lobe);

    contrib = bxdfEval.Radiance * abs(NdL) / max(1e-6, v1.PDF); // Note: Divide by sample PDF, not eval PDF

    pdf *= bxdfEval.PDF;

    if (FEAT_CORE(NEE))
    {
        pathState.LastBxdfPdf = bxdfEval.PDF;

        // TODO: Multiple NEE samples

        // TODO: Emissive Jacobian

        float shadowFactor;
        float3 L_nee = EvaluateNEE(hitInfo, bxdf, gSettings.DirectNumSamples, wo, v1.LightSample, shadowFactor);

        DBG_OUTPUT3(L_nee, GD_NeeContrib);

        if (v1.ShadowFactor > 0.99f != shadowFactor > 0.99f)
        {
            isSymmetric = false;
            contrib = 0.0f;
            wi = NAN;
            DBG_OUTPUT1(1.0f, GD_RejectionNEE);
            return;
        }

        contrib += L_nee * pathState.Beta;
    }
}

#endif