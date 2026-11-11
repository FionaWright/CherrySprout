#ifndef H_HIT_SHIFTED_UNCONNECTED_H
#define H_HIT_SHIFTED_UNCONNECTED_H

#include "PathTracing/Shift/ShiftMapping/HR.hlsli"
#include "PathTracing/Shift/ShiftMapping/Hybrid.hlsli"
#include "PathTracing/NEE/SampleNEE.hlsli"

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