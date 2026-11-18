#ifndef H_EMISSION_SHIFTED_H
#define H_EMISSION_SHIFTED_H

#include "PathTracing/EvaluateHitEmission.hlsli"

void getMisWeights_Emissive(
	PathStateShifted pathStateShifted,
	float pdf_prefix_m,
	float pdf_prefix_s,
	float pdf_bxdf_m,
    float pdf_bxdf_s,
    float pdf_nee_m,
    float pdf_nee_s,
	float jacobian,

    out float mis_nee_s,
    out float mis_nee_m,
    out float mis_path)
{
	float mainPathPDF_BXDF = pdf_prefix_m * pdf_bxdf_m;
    float shiftedPathPDF_BXDF = jacobian * pdf_prefix_s * pdf_bxdf_s;

	if (!FEAT_CORE(NEE) || !pathStateShifted.IsUsingNeeMis)
	{
		// Selecting (Main) PDF
        mis_path = BalanceHeuristic(
            mainPathPDF_BXDF, shiftedPathPDF_BXDF,
            1, 1);

		mis_nee_s = 1.0f;
		mis_nee_m = 1.0f;

#ifdef GDPT_MIS_WEIGHT_PATH
        mis_path = GDPT_MIS_WEIGHT_PATH;
#endif
		return;
	}

    float mainPathPDF_NEE = pdf_prefix_m * pdf_nee_m;
    float shiftedPathPDF_NEE = jacobian * pdf_prefix_s * pdf_nee_s;

    //mis_path = BalanceHeuristic(
    //    mainPathPDF_BXDF, mainPathPDF_NEE, shiftedPathPDF_BXDF, shiftedPathPDF_NEE,
    //    1, gSettings.DirectNumSamples, 1, gSettings.DirectNumSamples);

    //mis_path = BalanceHeuristic(
    //    pdf_prefix_m, jacobian * pdf_prefix_s,
    //    1, 1);

	mis_path = BalanceHeuristic(
        mainPathPDF_BXDF, shiftedPathPDF_BXDF,
        1, 1);

	mis_nee_m = BalanceHeuristic(
        pdf_bxdf_m, pdf_nee_m,
        1, gSettings.DirectNumSamples);

	mis_nee_s = BalanceHeuristic(
        pdf_bxdf_s, pdf_nee_s,
        1, gSettings.DirectNumSamples);

#ifdef GDPT_MIS_WEIGHT_PATH
    mis_path = GDPT_MIS_WEIGHT_PATH;
#endif
#ifdef GDPT_MIS_WEIGHT_NEE
	mis_nee_m = GDPT_MIS_WEIGHT_NEE;
	mis_nee_s = GDPT_MIS_WEIGHT_NEE;
#endif
}

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

    float pdf_bxdf_m = v0.PDF_BXDF;
    float pdf_bxdf_s = pathState.LastBxdfPdf;

    // TODO: Division is inefficient
    //float mainPathPrefixPDF = exp(pathStateShifted.LogPathPdfMain) / mainPDF_BXDF;
    //float shiftedPathPrefixPDF = exp(pathStateShifted.LogPathPdfShifted) / shiftPDF_BXDF;

    float pdf_prefix_m = pathStateShifted.LogPathPdfMain / pdf_bxdf_m;
    float pdf_prefix_s = pathStateShifted.LogPathPdfShifted / pdf_bxdf_s;

	float pdf_nee = v1.PDF_NEE;

	float mis_nee_s;
	float mis_nee_m;
    float mis_path;
    getMisWeights_Emissive(pathStateShifted, pdf_prefix_m, pdf_prefix_s, pdf_bxdf_m, pdf_bxdf_s, pdf_nee, pdf_nee, pathStateShifted.Jacobian, mis_nee_s, mis_nee_m, mis_path);

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

    float pdf_bxdf_m = v0.PDF_BXDF;
    float pdf_bxdf_s = pathState.LastBxdfPdf;

    // TODO: Division is inefficient
    //float mainPathPrefixPDF = exp(pathStateShifted.LogPathPdfMain) / mainPDF_BXDF;
    //float shiftedPathPrefixPDF = exp(pathStateShifted.LogPathPdfShifted) / shiftPDF_BXDF;

    float pdf_prefix_m = pathStateShifted.LogPathPdfMain / pdf_bxdf_m;
    float pdf_prefix_s = pathStateShifted.LogPathPdfShifted / pdf_bxdf_s;

	float pdf_nee_m = v1.PDF_NEE;
	float pdf_nee_s = NAN;
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
        pdf_nee_s = pdf_lsd * EvaluateEmissivePdf(hitInfo.PrimitiveCount, hitInfo.TriangleArea, hitInfo.Ng_ff, wo, hitInfo.RayT);
	}

	float mis_nee_s;
	float mis_nee_m;
    float mis_path;
    getMisWeights_Emissive(pathStateShifted, pdf_prefix_m, pdf_prefix_s, pdf_bxdf_m, pdf_bxdf_s, pdf_nee_m, pdf_nee_s, pathStateShifted.Jacobian, mis_nee_s, mis_nee_m, mis_path);

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

    float pdf_bxdf_m = v0.PDF_BXDF;
    float pdf_bxdf_s = pathState.LastBxdfPdf;

    // TODO: Division is inefficient
    //float mainPathPrefixPDF = exp(pathStateShifted.LogPathPdfMain) / mainPDF_BXDF;
    //float shiftedPathPrefixPDF = exp(pathStateShifted.LogPathPdfShifted) / shiftPDF_BXDF;

    float pdf_prefix_m = pathStateShifted.LogPathPdfMain / pdf_bxdf_m;
    float pdf_prefix_s = pathStateShifted.LogPathPdfShifted / pdf_bxdf_s;

	float pdf_nee_m = v1.PDF_NEE;
	float pdf_nee_s = pdfShifted_NEE;

	float mis_nee_s;
	float mis_nee_m;
    float mis_path;
    getMisWeights_Emissive(pathStateShifted, pdf_prefix_m, pdf_prefix_s, pdf_bxdf_m, pdf_bxdf_s, pdf_nee_m, pdf_nee_s, pathStateShifted.Jacobian, mis_nee_s, mis_nee_m, mis_path);

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