#ifndef H_HIT_SHIFTED_UNCONNECTED_H
#define H_HIT_SHIFTED_UNCONNECTED_H

#include "PathTracing/Shift/ShiftMapping/HR.hlsli"
#include "PathTracing/Shift/ShiftMapping/Hybrid.hlsli"
#include "PathTracing/NEE/SampleNEE.hlsli"

// TODO: Consider changed made for EvaluateSubPath and make sure they aren't affecting GDPT (PDF separate, MIS separate)

bool HitShiftedUnconnected(
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

    // TODO: Hybrid shift re-uses light sample RNG?

    // TODO: Multiple NEE samples

    // TODO: Emissive Jacobian

    if (FEAT_CORE(NEE) && v1.ShadowFactor > 0.0f)
    {
        // TODO: All lobe tags or only v1 lobe?
        // TODO: Shadow factor not applied (and for nee lobe tag part)

        float shadowFactor;
        float pdf_nee;
        float mis_nee;
        LightEval lightEval;
        float3 radiance = EvaluateNEE(hitInfo, gSettings.DirectNumSamples, wo, v1.LightSample, LobeTag::eAll, shadowFactor, pdf_nee, mis_nee, lightEval);

        bool isEmissiveLight = v1.LightSample.Index >= gSettings.LsdBaseEmissives;
        if (isEmissiveLight)
        {
            float3 x1 = v1.Position;
            float3 y1 = hitInfo.HitPos;
            float3 x2 = y1 + lightEval.Direction * lightEval.Distance;
            float3 y1_Ng = hitInfo.Ng_ff;
            float3 x2_Ng = lightEval.Ng;
            ShiftResult shiftResultEmissive = ShiftReconnect(x1, y1, y1_Ng, x2, x2_Ng);

            if (!shiftResultEmissive.IsSuccessful)
            {
                pathStateShifted.IsSymmetric = false;
                DBG_OUTPUT1(1.0f, GD_RejectionNEE);
                return false;
            }

            //DBG_FORCE_OUTPUT1(shiftResultEmissive.Jacobian);

            //pdf_nee *= shiftResultEmissive.Jacobian;
            pdf_nee /= shiftResultEmissive.Jacobian;

            // TODO:
            BxdfEval bxdfEval = BxDF::Evaluate(hitInfo, wo, lightEval.Direction, LobeTag::eAll);
            mis_nee = PowerHeuristic(pdf_nee, bxdfEval.PDF, gSettings.DirectNumSamples, 1);
            //mis_nee = 1.0f;
        }

        float3 E_direct = shadowFactor * mis_nee * radiance / max(1e-6f, pdf_nee);
        DBG_OUTPUT3(E_direct, GD_NeeContrib);

        if (abs(v1.ShadowFactor - shadowFactor) > EPSILON)
        {
            pathStateShifted.IsSymmetric = false;
            DBG_OUTPUT1(1.0f, GD_RejectionNEE);
            return false;
        }

        float3 L_sample = E_direct * pathState.Beta;
        pathState.Lo += ApplyFireflyThreshold(L_sample, gSettings.FireflyThreshold);
    }

    BxdfEval bxdfEval;
    ShiftResult shiftResult;
    if (FEAT_CORE(HybridShift))
        shiftResult = ShiftMapHybrid(pathStateShifted.ReconnectionState, hitInfo, v1, v2, wo, pathState.RaySegmentIdx, bxdfEval);
    else
        shiftResult = ShiftMapHPlusR(pathStateShifted.ReconnectionState, hitInfo, v1, v2, wo, pathState.RaySegmentIdx, bxdfEval);

    // TODO: if min(jacobian, 1.0 / jacobian) < 0.1f reject? (https://github.com/elite-sheep/gradient-restir/blob/release/Source/RenderPasses/GPathTracer/GPathTracer.slang)

    if (!shiftResult.IsSuccessful || bxdfEval.PDF <= 0.0f)
    {
        pathStateShifted.IsSymmetric = false;
        DBG_OUTPUT1(1.0f, GD_RejectionShiftMapping);
        return false;
    }

    wi = shiftResult.Wi;
    pathStateShifted.Jacobian *= shiftResult.Jacobian;
    DBG_OUTPUT1(shiftResult.Jacobian, GD_Jacobian);

    float NdL = dot(hitInfo.Ns_ff, wi);
    contrib = bxdfEval.Radiance * abs(NdL) / max(1e-6, v1.PDF_BXDF); // Note: Divide by sample PDF, not eval PDF

    pdf *= bxdfEval.PDF;

    if (FEAT_CORE(NEE))
        pathState.LastBxdfPdf = bxdfEval.PDF;

    return true;
}

#endif