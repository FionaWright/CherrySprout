#ifndef H_HIT_SHIFTED_UNCONNECTED_H
#define H_HIT_SHIFTED_UNCONNECTED_H

#include "PathTracing/Shift/ShiftMapping/HR.hlsli"
#include "PathTracing/Shift/ShiftMapping/Hybrid.hlsli"
#include "PathTracing/NEE/SampleNEE.hlsli"

bool SampleIndirect_S_Connected(
    inout PathState pathState,
    inout PathStateShifted pathStateShifted,

    PathVertexInfo v1,

    out float3 wi,
    out float pdfShifted)
{
    wi = v1.Wi;
    pdfShifted = v1.PDF_BXDF;

    pathState.Beta *= v1.IndirectContribution / max(1e-6f, v1.PDF_BXDF); // TODO: Missing something? :)
    return true;
}

bool SampleIndirect_S_Semiconnected(
    inout PathState pathState,
    inout PathStateShifted pathStateShifted,

    PathVertexInfo v1,
    HitInfo hitInfo,
    float3 wo,

    out float3 wi,
    out float pdfShifted)
{
    wi = v1.Wi;

    BxdfEval bxdfEval = BxDF::Evaluate(hitInfo, wo, wi, v1.Lobe);
    pdfShifted = bxdfEval.PDF;

    float NdL = dot(hitInfo.Ns_ff, wi);
    pathState.Beta *= bxdfEval.Radiance * abs(NdL) / max(1e-6f, v1.PDF_BXDF); // TODO: NDL?

    pathStateShifted.ReconnectionState = ReconnectionState::eConnected;
    return true;
}

bool SampleIndirect_S_Unconnected(
    inout PathState pathState,
    inout PathStateShifted pathStateShifted,

    PathVertexList mainVertices,
    PathVertexInfo v1,
    HitInfo hitInfo,
    float3 wo,

    out float3 wi,
	out float pdfShifted)
{
    pdfShifted = 1.0f;
    wi = NAN;

    PathVertexInfo v2 = mainVertices.Array[pathState.RaySegmentIdx + 1];
    DBG_OUTPUT3(Palette((uint)v2.Type), GD_V2Type);
    DBG_OUTPUT3(Palette((uint)v2.Lobe), GD_V2Lobe);

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

    pdfShifted = bxdfEval.PDF;

    float NdL = dot(hitInfo.Ns_ff, wi);
    float3 E_indirect = bxdfEval.Radiance * abs(NdL) / max(1e-6, v1.PDF_BXDF);

    pathState.Beta *= E_indirect;

    if (FEAT_CORE(NEE))
        pathState.LastBxdfPdf = bxdfEval.PDF;

    return true;
}

bool SampleIndirect_S(
    inout PathState pathState,
    inout PathStateShifted pathStateShifted,

    PathVertexList mainVertices,
    PathVertexInfo v1,
    HitInfo hitInfo,
    float3 wo,

    out float3 wi,
    out float pdfShifted)
{
    if (pathStateShifted.ReconnectionState == ReconnectionState::eUnconnected)
        return SampleIndirect_S_Unconnected(pathState, pathStateShifted, mainVertices, v1, hitInfo, wo, wi, pdfShifted);

    if (pathStateShifted.ReconnectionState == ReconnectionState::eSemiConnected)
        return SampleIndirect_S_Semiconnected(pathState, pathStateShifted, v1, hitInfo, wo, wi, pdfShifted);

    // TODO: Assert connected

    return SampleIndirect_S_Connected(pathState, pathStateShifted, v1, wi, pdfShifted);
}

#endif