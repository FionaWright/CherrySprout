#ifndef H_GD_MISS_SHIFTED_H
#define H_GD_MISS_SHIFTED_H

#include "PathTracing/Structs.h"
#include "PathTracing/MIS.hlsli"

// TODO: Furnace Tests, dir lights

void Miss_S_Connected(
    inout PathState pathState,
    inout PathStateShifted pathStateShifted,

    PathVertexList mainVertices,
    PathVertexInfo v1)
{
    // Note: Not possible to be connected on primary ray

    PathVertexInfo v0 = mainVertices.Array[pathState.RaySegmentIdx-1];

    float pdf_bxdf_m = v0.PDF_BXDF;
    float pdf_bxdf_s = pathState.LastBxdfPdf;

    float pdf_nee = v1.PDF_NEE;

    float pdf_prefix_m = pathStateShifted.PathPdfMain / pdf_bxdf_m;
    float pdf_prefix_s = pathStateShifted.PathPdfShifted / pdf_bxdf_s;

    float mis_nee_s;
    float mis_nee_m;
    float mis_path;
    getMisWeights_BxdfSubpath(pathStateShifted, pdf_prefix_m, pdf_prefix_s, pdf_bxdf_m, pdf_bxdf_s, pdf_nee, pdf_nee, pathStateShifted.Jacobian, mis_nee_s, mis_nee_m, mis_path);

    float3 shiftedLo = mis_nee_s * pathState.Beta * v1.DirectContribution;
    shiftedLo = ApplyFireflyThreshold(shiftedLo, gSettings.FireflyThreshold);

    pathState.Lo = shiftedLo;

    shiftedLo *= pathStateShifted.Jacobian;

    float3 mainLo = mis_nee_m * v1.InitialBeta * v1.DirectContribution;
    mainLo = ApplyFireflyThreshold(mainLo, gSettings.FireflyThreshold);

    pathStateShifted.SubpathGradients += (shiftedLo - mainLo) * mis_path;
}

void Miss_S_Semi_or_Unconnected(
    inout PathState pathState,
    inout PathStateShifted pathStateShifted,

    PathVertexList mainVertices,
    PathVertexInfo v1)
{
    float _, pdf_nee_s;
    float3 Li = Miss(pathState, _, pdf_nee_s);

    if (pathState.GetPathFlag(PathFlags::eIsPrimaryRay))
    {
        // Note: beta, jacobian always 1
        float3 Li_s = ApplyFireflyThreshold(Li, gSettings.FireflyThreshold);
        float3 Li_m = ApplyFireflyThreshold(v1.DirectContribution, gSettings.FireflyThreshold);

        pathState.Lo = Li_s;

        float mis_path = BalanceHeuristic(1.0f, 1.0f, 1, 1);
        pathStateShifted.SubpathGradients += (Li_s - Li_m) * mis_path;
        return;
    }

    PathVertexInfo v0 = mainVertices.Array[pathState.RaySegmentIdx-1];

    float pdf_bxdf_m = v0.PDF_BXDF;
    float pdf_bxdf_s = pathState.LastBxdfPdf;

    // TODO: Division is inefficient
    float pdf_prefix_m = pathStateShifted.PathPdfMain / pdf_bxdf_m;
    float pdf_prefix_s = pathStateShifted.PathPdfShifted / pdf_bxdf_s;

    float pdf_nee_m = v1.PDF_NEE;

    float mis_nee_s;
    float mis_nee_m;
    float mis_path;
    getMisWeights_BxdfSubpath(pathStateShifted, pdf_prefix_m, pdf_prefix_s, pdf_bxdf_m, pdf_bxdf_s, pdf_nee_m, pdf_nee_s, pathStateShifted.Jacobian, mis_nee_s, mis_nee_m, mis_path);

    float3 shiftedLo = mis_nee_s * pathState.Beta * Li;
    shiftedLo = ApplyFireflyThreshold(shiftedLo, gSettings.FireflyThreshold);

    pathState.Lo = shiftedLo;

    shiftedLo *= pathStateShifted.Jacobian;

    float3 mainLo = mis_nee_m * v1.InitialBeta * v1.DirectContribution;
    mainLo = ApplyFireflyThreshold(mainLo, gSettings.FireflyThreshold);

    pathStateShifted.SubpathGradients += (shiftedLo - mainLo) * mis_path;
}

void Miss_S(
    inout PathState pathState,
    inout PathStateShifted pathStateShifted,

    PathVertexList mainVertices,
    PathVertexInfo v1)
{
    if (pathStateShifted.ReconnectionState == ReconnectionState::eConnected)
    {
        Miss_S_Connected(pathState, pathStateShifted, mainVertices, v1);
    }
    else
    {
        Miss_S_Semi_or_Unconnected(pathState, pathStateShifted, mainVertices, v1);
    }
}

#endif