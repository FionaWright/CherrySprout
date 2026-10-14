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

    if (v2.Type == VertexType::eUninitialized)
    {
        isSymmetric = false;
        contrib = 1.0f;
        wi = NAN;
        DBG_OUTPUT1(1.0f, GD_RejectionUninit);
        return;
    }

    VertexType shiftedType = GetIsVertexDiffuse(hitInfo.Mat.Roughness) ? VertexType::eDiffuse : VertexType::eGlossy;
    if (v1.Type != shiftedType || v2.Type == VertexType::eGlossy)
    {
        isSymmetric = false;
        contrib = 1.0f;
        wi = NAN;
        DBG_OUTPUT1(1.0f, GD_RejectionVertexMismatch);
        return;
    }

    ShiftResult shiftResult = (ShiftResult)0;

    if (v1.Type == VertexType::eDiffuse)
    {
        if (v2.Type == VertexType::eDiffuse || v2.Type == VertexType::eEmission)
            shiftResult = ShiftReconnect(v1.Position, hitInfo.HitPosBiased, hitInfo.Ns_ff, v2.Position, v2.SFrame.N);
        else if (v2.Type == VertexType::eEnvironment)
            shiftResult = ShiftReconnectEnvironment(hitInfo.HitPosBiased, hitInfo.Ns_ff, v1.Wi);
		else
        	DBG_ASSERT_FAIL(GD_INVALID_VERTEX_TYPE);

        reconnectionState = ReconnectionState::eSemiConnected;
    }
    else if (v1.Type == VertexType::eGlossy)
    {
        float iorNCurrent =  hitInfo.IsEntering ? IOR_N_AIR          : hitInfo.Mat.IOR_N;
        float iorNNext =     hitInfo.IsEntering ? hitInfo.Mat.IOR_N  : IOR_N_AIR;
        float eta = iorNCurrent / iorNNext;

        shiftResult = ShiftHalfVector(v1.SFrame, hitInfo.SFrame, v1.Wi, v1.Wo, wo, v1.Eta, eta);
    }
    else
        DBG_ASSERT_FAIL(GD_INVALID_VERTEX_TYPE);

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
    BxdfEval bxdfEval = bxdf.Evaluate(hitInfo, wo, wi);

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