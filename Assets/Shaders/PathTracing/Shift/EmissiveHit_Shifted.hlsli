#ifndef H_EMISSION_SHIFTED_H
#define H_EMISSION_SHIFTED_H

#include "PathTracing/EvaluateHitEmission.hlsli"

bool EmissiveHit_S_Connected(
    inout PathState pathState,
    inout PathStateShifted pathStateShifted,

    PathVertexList mainVertices,
    PathVertexInfo v1)
{
    DBG_OUTPUT3(v1.DirectContribution, GD_Emission);

    bool isEmissive = v1.Type == VertexType::eEmission;
    if (!isEmissive)
        return true;

    // Note: Cannot be primary ray when connected

    PathVertexInfo v0 = mainVertices.Array[pathState.RaySegmentIdx-1];

    float mainPDF_BXDF = v0.PDF_BXDF;
    float shiftPDF_BXDF = pathState.LastBxdfPdf;

    // TODO: Division is inefficient
    //float mainPathPrefixPDF = exp(pathStateShifted.LogPathPdfMain) / mainPDF_BXDF;
    //float shiftedPathPrefixPDF = exp(pathStateShifted.LogPathPdfShifted) / shiftPDF_BXDF;

    float mainPathPrefixPDF = pathStateShifted.LogPathPdfMain / mainPDF_BXDF;
    float shiftedPathPrefixPDF = pathStateShifted.LogPathPdfShifted / shiftPDF_BXDF;

    float mainPathPDF_BXDF = mainPathPrefixPDF * mainPDF_BXDF;
    float shiftedPathPDF_BXDF = pathStateShifted.Jacobian * shiftedPathPrefixPDF * shiftPDF_BXDF;

	float mis_nee_s = 1.0f;
	float mis_nee_m = 1.0f;
    float mis_path = 1.0f;
    if (FEAT_CORE(NEE) && pathStateShifted.IsUsingNeeMis)
    {
        float PDF_NEE = v1.PDF_NEE;

        float mainPathPDF_NEE = mainPathPrefixPDF * PDF_NEE;
        float shiftedPathPDF_NEE = pathStateShifted.Jacobian * shiftedPathPrefixPDF * PDF_NEE;

        // Selecting (Main, BxDF) PDF
        //mis_path = BalanceHeuristic(
        //    mainPathPDF_BXDF, mainPathPDF_NEE, shiftedPathPDF_NEE, shiftedPathPDF_BXDF,
        //    1, gSettings.DirectNumSamples, gSettings.DirectNumSamples, 1);

		// Selecting (Main, BxDF) PDF
        mis_path = BalanceHeuristic(
            mainPathPrefixPDF, pathStateShifted.Jacobian * shiftedPathPrefixPDF,
            1, 1);

		mis_nee_m = BalanceHeuristic(
            mainPDF_BXDF, PDF_NEE,
            1, 1); // TODO: Multiple samples

		mis_nee_s = BalanceHeuristic(
            shiftPDF_BXDF, PDF_NEE,
            1, 1);

#ifdef GDPT_MIS_WEIGHT
        mis_path = GDPT_MIS_WEIGHT;
#endif
    }
    else
    {
        // Selecting (Main) PDF
        mis_path = BalanceHeuristic(
            mainPathPDF_BXDF, shiftedPathPDF_BXDF,
            1, 1);

#ifdef GDPT_MIS_WEIGHT_NO_NEE
        mis_path = GDPT_MIS_WEIGHT_NO_NEE;
#endif
    }

    float3 mainLo = mis_nee_m * v1.InitialBeta * v1.DirectContribution;
    mainLo = ApplyFireflyThreshold(mainLo, gSettings.FireflyThreshold);

    float3 shiftedLo = mis_nee_s * pathState.Beta * v1.DirectContribution;
    shiftedLo = ApplyFireflyThreshold(shiftedLo, gSettings.FireflyThreshold);

    pathState.Lo = shiftedLo;

    shiftedLo *= pathStateShifted.Jacobian;

    pathStateShifted.SubpathGradients += (shiftedLo - mainLo) * mis_path;
    return false;
}

bool EmissiveHit_S_Semiconnected(
    inout PathState pathState,
    inout PathStateShifted pathStateShifted,

    HitInfo hitInfo,
    PathVertexList mainVertices,
    PathVertexInfo v1)
{
    float3 wo = -pathState.Desc.Direction;

    DBG_OUTPUT3(v1.DirectContribution, GD_Emission);

    bool isEmissive = v1.Type == VertexType::eEmission;
    if (!isEmissive)
        return true;

    PathVertexInfo v0 = mainVertices.Array[pathState.RaySegmentIdx-1];

    float mainPDF_BXDF = v0.PDF_BXDF;
    float shiftPDF_BXDF = pathState.LastBxdfPdf;

    // TODO: Division is inefficient
    //float mainPathPrefixPDF = exp(pathStateShifted.LogPathPdfMain) / mainPDF_BXDF;
    //float shiftedPathPrefixPDF = exp(pathStateShifted.LogPathPdfShifted) / shiftPDF_BXDF;

    float mainPathPrefixPDF = pathStateShifted.LogPathPdfMain / mainPDF_BXDF;
    float shiftedPathPrefixPDF = pathStateShifted.LogPathPdfShifted / shiftPDF_BXDF;

    float mainPathPDF_BXDF = mainPathPrefixPDF * mainPDF_BXDF;
    float shiftedPathPDF_BXDF = pathStateShifted.Jacobian * shiftedPathPrefixPDF * shiftPDF_BXDF;

	float mis_nee_s = 1.0f;
	float mis_nee_m = 1.0f;
    float mis_path = 1.0f;
    if (FEAT_CORE(NEE) && pathStateShifted.IsUsingNeeMis)
    {
        float pdf_lsd;
        {
            int emissiveIdx = gInstanceToEmissiveInstanceMap[hitInfo.InstanceIdx];
            DBG_ASSERT_NEQ(emissiveIdx, -1, EMISSIVE_NOT_MAPPED);
            uint lightIdx = emissiveIdx + gSettings.LsdBaseEmissives;
            EvaluateLSD(lightIdx, pdf_lsd);
        }

        // Note: wo is already from perspective of light
        float mainPDF_NEE = v1.PDF_NEE;
        float shiftPDF_NEE = pdf_lsd * EvaluateEmissivePdf(hitInfo.PrimitiveCount, hitInfo.TriangleArea, hitInfo.Ng_ff, wo, hitInfo.RayT);

        float mainPathPDF_NEE = mainPathPrefixPDF * mainPDF_NEE;
        float shiftedPathPDF_NEE = pathStateShifted.Jacobian * shiftedPathPrefixPDF * shiftPDF_NEE;

        // Selecting (Main, BxDF) PDF
        //mis_path = BalanceHeuristic(
        //    mainPathPDF_BXDF, mainPathPDF_NEE, shiftedPathPDF_NEE, shiftedPathPDF_BXDF,
        //    1, gSettings.DirectNumSamples, gSettings.DirectNumSamples, 1);

		// Selecting (Main, BxDF) PDF
        mis_path = BalanceHeuristic(
            mainPathPrefixPDF, pathStateShifted.Jacobian * shiftedPathPrefixPDF,
            1, 1);

		mis_nee_m = BalanceHeuristic(
            mainPDF_BXDF, mainPDF_NEE,
            1, 1); // TODO: Multiple samples

		mis_nee_s = BalanceHeuristic(
            shiftPDF_BXDF, shiftPDF_NEE,
            1, 1);

#ifdef GDPT_MIS_WEIGHT
        mis_path = GDPT_MIS_WEIGHT;
#endif
    }
    else
    {
        // Selecting (Main, BxDF) PDF
        mis_path = BalanceHeuristic(
            mainPathPDF_BXDF, shiftedPathPDF_BXDF,
            1, 1);

#ifdef GDPT_MIS_WEIGHT_NO_NEE
        mis_path = GDPT_MIS_WEIGHT_NO_NEE;
#endif
    }

    float3 mainLo = mis_nee_m * v1.InitialBeta * v1.DirectContribution;
    mainLo = ApplyFireflyThreshold(mainLo, gSettings.FireflyThreshold);

    float3 shiftedLo = mis_nee_s * pathState.Beta * v1.DirectContribution;
    shiftedLo = ApplyFireflyThreshold(shiftedLo, gSettings.FireflyThreshold);

    pathState.Lo = shiftedLo;

    shiftedLo *= pathStateShifted.Jacobian;

    pathStateShifted.SubpathGradients += (shiftedLo - mainLo) * mis_path;
    return false;
}

bool EmissiveHit_S_Unconnected(
    inout PathState pathState,
    inout PathStateShifted pathStateShifted,

    HitInfo hitInfo,
    PathVertexList mainVertices,
    PathVertexInfo v1)
{
    float3 wo = -pathState.Desc.Direction;

    float pdfShifted_NEE, _;
    float3 Li = EvaluateHitEmission(hitInfo, pathState, wo, pdfShifted_NEE, _);

    DBG_OUTPUT3(Li, GD_Emission);

    bool isEmissive = any(Li > 0.0f);
    bool mainIsEmissive = v1.Type == VertexType::eEmission;

    if (isEmissive != mainIsEmissive)
    {
        pathStateShifted.IsSymmetric = false;
        DBG_OUTPUT1(1.0f, GD_RejectionEmission);
        return false;
    }

    if (!isEmissive)
        return true;

    DBG_FORCE_OUTPUT3(COLOR_RED);

    if (pathState.GetPathFlag(PathFlags::eIsPrimaryRay))
    {
        // Note: beta, jacobian always 1
        float3 Li_s = ApplyFireflyThreshold(Li, gSettings.FireflyThreshold);
        float3 Li_m = ApplyFireflyThreshold(v1.DirectContribution, gSettings.FireflyThreshold);

        pathState.Lo = Li_s;

        float mis_path = BalanceHeuristic(1.0f, 1.0f, 1, 1);
#ifdef GDPT_MIS_WEIGHT
        mis_path = GDPT_MIS_WEIGHT;
#endif
        pathStateShifted.SubpathGradients += (Li_s - Li_m) * mis_path;
        return false;
    }

    PathVertexInfo v0 = mainVertices.Array[pathState.RaySegmentIdx-1];

    float mainPDF_BXDF = v0.PDF_BXDF;
    float shiftPDF_BXDF = pathState.LastBxdfPdf;

    // TODO: Division is inefficient
    //float mainPathPrefixPDF = exp(pathStateShifted.LogPathPdfMain) / mainPDF_BXDF;
    //float shiftedPathPrefixPDF = exp(pathStateShifted.LogPathPdfShifted) / shiftPDF_BXDF;

    float mainPathPrefixPDF = pathStateShifted.LogPathPdfMain / mainPDF_BXDF;
    float shiftedPathPrefixPDF = pathStateShifted.LogPathPdfShifted / shiftPDF_BXDF;

    float mainPathPDF_BXDF = mainPathPrefixPDF * mainPDF_BXDF;
    float shiftedPathPDF_BXDF = pathStateShifted.Jacobian * shiftedPathPrefixPDF * shiftPDF_BXDF;

	float mis_nee_s = 1.0f;
	float mis_nee_m = 1.0f;
    float mis_path = 1.0f;
    if (FEAT_CORE(NEE) && pathStateShifted.IsUsingNeeMis)
    {
        float mainPDF_NEE = v1.PDF_NEE;
        float shiftPDF_NEE = pdfShifted_NEE;

        float mainPathPDF_NEE = mainPathPrefixPDF * mainPDF_NEE;
        float shiftedPathPDF_NEE = pathStateShifted.Jacobian * shiftedPathPrefixPDF * shiftPDF_NEE;

        // Selecting (Main, BxDF) PDF
        //mis_path = BalanceHeuristic(
        //    mainPathPDF_BXDF, mainPathPDF_NEE, shiftedPathPDF_NEE, shiftedPathPDF_BXDF,
        //    1, gSettings.DirectNumSamples, gSettings.DirectNumSamples, 1);

		// Selecting (Main) PDF
        mis_path = BalanceHeuristic(
            mainPathPrefixPDF, pathStateShifted.Jacobian * shiftedPathPrefixPDF,
            1, 1);

		mis_nee_m = BalanceHeuristic(
            mainPDF_BXDF, mainPDF_NEE,
            1, 1); // TODO: Multiple samples

		mis_nee_s = BalanceHeuristic(
            shiftPDF_BXDF, shiftPDF_NEE,
            1, 1);

#ifdef GDPT_MIS_WEIGHT
        mis_path = GDPT_MIS_WEIGHT;
#endif
    }
    else
    {
        // Selecting (Main) PDF
        mis_path = BalanceHeuristic(
            mainPathPDF_BXDF, shiftedPathPDF_BXDF,
            1, 1);

#ifdef GDPT_MIS_WEIGHT_NO_NEE
        mis_path = GDPT_MIS_WEIGHT_NO_NEE;
#endif
    }

    float3 mainLo = mis_nee_m * v1.InitialBeta * v1.DirectContribution;
    mainLo = ApplyFireflyThreshold(mainLo, gSettings.FireflyThreshold);

    float3 shiftedLo = mis_nee_s * pathState.Beta * Li;
    shiftedLo = ApplyFireflyThreshold(shiftedLo, gSettings.FireflyThreshold);

    pathState.Lo = shiftedLo;

    shiftedLo *= pathStateShifted.Jacobian;

    pathStateShifted.SubpathGradients += (shiftedLo - mainLo) * mis_path;
    return false;
}

bool EmissiveHit_S(
    inout PathState pathState,
    inout PathStateShifted pathStateShifted,

    HitInfo hitInfo,
    PathVertexList mainVertices,
    PathVertexInfo v1)
{
    if (pathStateShifted.ReconnectionState == ReconnectionState::eConnected)
    {
        return EmissiveHit_S_Connected(pathState, pathStateShifted, mainVertices, v1);
    }
    else if (pathStateShifted.ReconnectionState == ReconnectionState::eSemiConnected)
    {
        return EmissiveHit_S_Semiconnected(pathState, pathStateShifted, hitInfo, mainVertices, v1);
    }
    {
        return EmissiveHit_S_Unconnected(pathState, pathStateShifted, hitInfo, mainVertices, v1);
    }
}

#endif