#ifndef H_HIT_SHIFTED_UNCONNECTED_H
#define H_HIT_SHIFTED_UNCONNECTED_H

#include "PathTracing/ShiftMapping/HR.hlsli"
#include "PathTracing/ShiftMapping/Hybrid.hlsli"

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

    BxdfEval bxdfEval;
    ShiftResult shiftResult;
    if (FEAT_CORE(GdptHybridShift))
        shiftResult = ShiftMapHybrid(reconnectionState, hitInfo, v1, v2, wo, pathState.RaySegmentIdx, bxdfEval);
    else
        shiftResult = ShiftMapHPlusR(reconnectionState, hitInfo, v1, v2, wo, pathState.RaySegmentIdx, bxdfEval);

    if (!shiftResult.IsSuccessful || bxdfEval.PDF <= 0.0f)
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

    contrib = bxdfEval.Radiance * abs(NdL) / max(1e-6, v1.PDF); // Note: Divide by sample PDF, not eval PDF

    pdf *= bxdfEval.PDF;

    // TODO: Hybrid shift re-uses light sample RNG?

    if (FEAT_CORE(NEE))
    {
        pathState.LastBxdfPdf = bxdfEval.PDF;

        // TODO: Multiple NEE samples

        // TODO: Emissive Jacobian

        float shadowFactor;
        float3 L_nee = EvaluateNEE(hitInfo, gSettings.DirectNumSamples, wo, v1.LightSample, shadowFactor);

        DBG_OUTPUT3(L_nee, GD_NeeContrib);

        // TODO: Should this even be trying NEE samples if they failed for main path? Just going to reject anyways
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