#ifndef H_GD_MISS_SHIFTED_H
#define H_GD_MISS_SHIFTED_H

#include "PathTracing/Structs.h"
#include "PathTracing/MIS.hlsli"

// TODO: Furnace Tests, dir lights

float3 Miss_S_Connected(
    inout PathState pathState,
    inout PathStateShifted pathStateShifted,

    PathVertexList mainVertices,
    PathVertexInfo v1)
{
    // Note: Not possible to be connected on primary ray

    PathVertexInfo v0 = mainVertices.Array[pathState.RaySegmentIdx-1];

    float mainPDF_BXDF = v0.PDF_BXDF;
    float shiftPDF_BXDF = pathState.LastBxdfPdf;

    float mainPathPrefixPDF = exp(pathStateShifted.LogPathPdfMain) / mainPDF_BXDF;
    float shiftedPathPrefixPDF = exp(pathStateShifted.LogPathPdfShifted) / shiftPDF_BXDF;

    float mainPathPDF_BXDF = mainPathPrefixPDF * mainPDF_BXDF;
    float shiftedPathPDF_BXDF = pathStateShifted.Jacobian * shiftedPathPrefixPDF * shiftPDF_BXDF;

    float mis_path = 1.0f;
    if (FEAT_CORE(NEE) && pathStateShifted.IsUsingNeeMis)
    {
        // Note: When connected, always has same wi
        float pdf_lsd;
        EvaluateLSD(0, pdf_lsd);
        float2 uv = EaSphereToSquare(pathState.Desc.Direction);
        float PDF_NEE = pdf_lsd * EvaluateEnvMapPdf(uv);

        float mainPathPDF_NEE = mainPathPrefixPDF * PDF_NEE;
        float shiftedPathPDF_NEE = pathStateShifted.Jacobian * shiftedPathPrefixPDF * PDF_NEE;

        // Selecting (Main, BxDF) PDF
        mis_path = BalanceHeuristic(
            mainPathPDF_BXDF, mainPathPDF_NEE, shiftedPathPDF_NEE, shiftedPathPDF_BXDF,
            1, gSettings.DirectNumSamples, gSettings.DirectNumSamples, 1);
    }
    else
    {
        // Selecting (Main, BxDF) PDF
        mis_path = BalanceHeuristic(
            mainPathPDF_BXDF, shiftedPathPDF_BXDF,
            1, 1);
    }

    float3 mainLo = v1.InitialBeta * v1.DirectContribution;
    mainLo = ApplyFireflyThreshold(mainLo, gSettings.FireflyThreshold);

    float3 shiftedLo = pathStateShifted.Jacobian * pathState.Beta * v1.DirectContribution;
    shiftedLo = ApplyFireflyThreshold(shiftedLo, gSettings.FireflyThreshold);

    pathState.Lo = shiftedLo;

    return (shiftedLo - mainLo) * mis_path;
}

float3 Miss_S_Semi_or_Unconnected(
    inout PathState pathState,
    inout PathStateShifted pathStateShifted,

    PathVertexList mainVertices,
    PathVertexInfo v1)
{
    float _; // Discarded
    float3 Li = Miss(pathState, _);

    if (pathState.GetPathFlag(PathFlags::eIsPrimaryRay))
    {
        // Note: beta, jacobian always 1
        float3 Li_s = ApplyFireflyThreshold(Li, gSettings.FireflyThreshold);
        float3 Li_m = ApplyFireflyThreshold(v1.DirectContribution, gSettings.FireflyThreshold);

        float mis_path = BalanceHeuristic(1.0f, 1.0f, 1, 1);
        return (Li_s - Li_m) * mis_path;
    }

    PathVertexInfo v0 = mainVertices.Array[pathState.RaySegmentIdx-1];

    float mainPDF_BXDF = v0.PDF_BXDF;
    float shiftPDF_BXDF = pathState.LastBxdfPdf;

    // TODO: Division is inefficient
    float mainPathPrefixPDF = exp(pathStateShifted.LogPathPdfMain) / mainPDF_BXDF;
    float shiftedPathPrefixPDF = exp(pathStateShifted.LogPathPdfShifted) / shiftPDF_BXDF;

    float mainPathPDF_BXDF = mainPathPrefixPDF * mainPDF_BXDF;
    float shiftedPathPDF_BXDF = pathStateShifted.Jacobian * shiftedPathPrefixPDF * shiftPDF_BXDF;

    float mis_path = 1.0f;
    if (FEAT_CORE(NEE) && pathStateShifted.IsUsingNeeMis)
    {
        float pdf_lsd;
        EvaluateLSD(0, pdf_lsd);

        float2 uv_main = EaSphereToSquare(v0.Wi);
        float2 uv_shift = EaSphereToSquare(pathState.Desc.Direction);

        float mainPDF_NEE = pdf_lsd * EvaluateEnvMapPdf(uv_main);
        float shiftPDF_NEE = pdf_lsd * EvaluateEnvMapPdf(uv_shift);

        float mainPathPDF_NEE = mainPathPrefixPDF * mainPDF_NEE;
        float shiftedPathPDF_NEE = pathStateShifted.Jacobian * shiftedPathPrefixPDF * shiftPDF_NEE;

        // Selecting (Main, BxDF) PDF
        mis_path = BalanceHeuristic(
            mainPathPDF_BXDF, mainPathPDF_NEE, shiftedPathPDF_NEE, shiftedPathPDF_BXDF,
            1, gSettings.DirectNumSamples, gSettings.DirectNumSamples, 1);
    }
    else
    {
        // Selecting (Main, BxDF) PDF
        mis_path = BalanceHeuristic(
            mainPathPDF_BXDF, shiftedPathPDF_BXDF,
            1, 1);
    }

    float3 mainLo = v1.InitialBeta * v1.DirectContribution;
    mainLo = ApplyFireflyThreshold(mainLo, gSettings.FireflyThreshold);

    float3 shiftedLo = pathStateShifted.Jacobian * pathState.Beta * Li;
    shiftedLo = ApplyFireflyThreshold(shiftedLo, gSettings.FireflyThreshold);

    pathState.Lo = shiftedLo;

    //DBG_FORCE_OUTPUT3((shiftedLo - mainLo));

    return (shiftedLo - mainLo) * mis_path;
}

void Miss_S(
    inout PathState pathState,
    inout PathStateShifted pathStateShifted,

    PathVertexList mainVertices,
    PathVertexInfo v1)
{
    float3 gradient;

    if (pathStateShifted.ReconnectionState == ReconnectionState::eConnected)
    {
        gradient = Miss_S_Connected(pathState, pathStateShifted, mainVertices, v1);
    }
    else
    {
        gradient = Miss_S_Semi_or_Unconnected(pathState, pathStateShifted, mainVertices, v1);
    }

    pathStateShifted.SubpathGradients += gradient;
}

#endif