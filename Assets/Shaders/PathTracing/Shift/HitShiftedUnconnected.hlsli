#ifndef H_HIT_SHIFTED_UNCONNECTED_H
#define H_HIT_SHIFTED_UNCONNECTED_H

#include "PathTracing/Shift/ShiftMapping/HR.hlsli"
#include "PathTracing/Shift/ShiftMapping/Hybrid.hlsli"

void HitShiftedUnconnected(
    inout PathState pathState,
    inout PathStateShifted pathStateShifted,

    PathVertexInfo v1,
    PathVertexInfo v2,
    HitInfo hitInfo,
    float3 wo,

    out float3 wi,
    out float3 contrib,
	out float pdf)
{
    pdf = 1.0f;
    contrib = NAN;
    wi = NAN;

    BxdfEval bxdfEval;
    ShiftResult shiftResult;
    if (FEAT_CORE(HybridShift))
        shiftResult = ShiftMapHybrid(pathStateShifted.ReconnectionState, hitInfo, v1, v2, wo, pathState.RaySegmentIdx, bxdfEval);
    else
        shiftResult = ShiftMapHPlusR(pathStateShifted.ReconnectionState, hitInfo, v1, v2, wo, pathState.RaySegmentIdx, bxdfEval);

    if (!shiftResult.IsSuccessful || bxdfEval.PDF <= 0.0f)
    {
        pathStateShifted.IsSymmetric = false;
        DBG_OUTPUT1(1.0f, GD_RejectionShiftMapping);
        return;
    }

    wi = shiftResult.Wi;
    pathStateShifted.Jacobian *= shiftResult.Jacobian;
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

        if (v1.ShadowFactor > 0.0f)
        {
            float shadowFactor;
            float3 L_nee = EvaluateNEE(hitInfo, gSettings.DirectNumSamples, wo, v1.LightSample, shadowFactor);

            DBG_OUTPUT3(L_nee, GD_NeeContrib);

            // TODO: Should this even be trying NEE samples if they failed for main path? Just going to reject anyways
            if (shadowFactor <= 0.0f)
            {
                pathStateShifted.IsSymmetric = false;
                DBG_OUTPUT1(1.0f, GD_RejectionNEE);
                return;
            }

            contrib += L_nee * pathState.Beta;
        }
    }
}

#endif