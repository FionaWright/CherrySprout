#ifndef H_NEE_SHIFTED_UNCONNECTED_H
#define H_NEE_SHIFTED_UNCONNECTED_H

#include "PathTracing/Shift/ShiftMapping/Components/Reconnect.hlsli"
#include "PathTracing/NEE/Utils.hlsli"
#include "PathTracing/NEE/SampleNEE.hlsli"

bool SampleDirect_S_Connected(
    inout PathState pathState,
    inout PathStateShifted pathStateShifted,

    PathVertexInfo v1,
    HitInfo hitInfo,
    float3 wo)
{
    float jacobianPath = pathStateShifted.Jacobian; // Jacobian at vertex is 1

    //float mainPathPrefixPDF = exp(pathStateShifted.LogPathPdfMain);
    //float shiftedPathPrefixPDF = exp(pathStateShifted.LogPathPdfShifted);

    float mainPathPrefixPDF = pathStateShifted.LogPathPdfMain;
    float shiftedPathPrefixPDF = pathStateShifted.LogPathPdfShifted;

    float mainPDF_NEE = v1.PDF_NEE;
    float shiftPDF_NEE = v1.PDF_NEE;

    float mainPathPDF_NEE = mainPathPrefixPDF * mainPDF_NEE;
    float shiftedPathPDF_NEE = jacobianPath * shiftedPathPrefixPDF * shiftPDF_NEE;

	float mis_nee_m = 1.0f;
	float mis_nee_s = 1.0f;
    float mis_path = 1.0f;
    if (pathStateShifted.IsUsingNeeMis)
    {
        float PDF_BXDF = v1.PDF_BXDF_at_NEE;

        float mainPathPDF_BXDF = mainPathPrefixPDF * PDF_BXDF;
        float shiftedPathPDF_BXDF = jacobianPath * shiftedPathPrefixPDF * PDF_BXDF;

        // Selecting (Main, NEE) PDF
        //mis_path = BalanceHeuristic(
        //    mainPathPDF_NEE, mainPathPDF_BXDF, shiftedPathPDF_NEE, shiftedPathPDF_BXDF,
        //    gSettings.DirectNumSamples, 1, gSettings.DirectNumSamples, 1);

		mis_path = BalanceHeuristic(
            mainPathPrefixPDF, jacobianPath * shiftedPathPrefixPDF,
            gSettings.DirectNumSamples, gSettings.DirectNumSamples);

		mis_nee_m = BalanceHeuristic(
            mainPDF_NEE, PDF_BXDF,
            gSettings.DirectNumSamples, 1);

		mis_nee_s = BalanceHeuristic(
            shiftPDF_NEE, PDF_BXDF,
            gSettings.DirectNumSamples, 1);

#ifdef GDPT_MIS_WEIGHT
        mis_path = GDPT_MIS_WEIGHT;
#endif
    }
    else
    {
        // Selecting (Main, NEE) PDF
        mis_path = BalanceHeuristic(
            mainPathPDF_NEE, shiftedPathPDF_NEE,
            gSettings.DirectNumSamples, gSettings.DirectNumSamples);
    }


    // Note: No NEE/BxDF MIS weights applied here
    float3 vertexLo = v1.ShadowFactor * v1.DirectContribution / max(1e-6f, mainPDF_NEE);

    float3 mainLo = mis_nee_m * v1.InitialBeta * vertexLo;
    float3 shiftedLo = mis_nee_s * jacobianPath * pathState.Beta * vertexLo;

    pathState.Lo = shiftedLo;

    float3 gradient = (shiftedLo - mainLo) * mis_path; // Flipped at the end if backwards estimator
    pathStateShifted.SubpathGradients += gradient;

    DBG_OUTPUT3(mainLo, GD_Nee_MainLo);
    DBG_OUTPUT3(shiftedLo, GD_Nee_ShiftLo);
    DBG_OUTPUT3(gradient, GD_Nee_Gradient);
    DBG_OUTPUT1(mis_path, GD_Nee_MIS);
    DBG_OUTPUT1(jacobianPath, GD_Nee_Jacobian);

    return true;
}

bool SampleDirect_S_Semiconnected(
    inout PathState pathState,
    inout PathStateShifted pathStateShifted,

    PathVertexInfo v1,
    HitInfo hitInfo,
    float3 wo)
{
    if (v1.PDF_NEE <= 0.0f)
        return true;

    // TODO: The only things needed from here are the pdf and radiance (Which includes light eval radiance). Shadow factor is wasted computation
    float _sf;
    float _pdfnee, pdfShifted_BXDF;
    float _mis;
    LightEval _l;
    float3 radiance = EvaluateNEE(hitInfo, gSettings.DirectNumSamples, wo, v1.LightSample, LobeTag::eAll, _sf, _pdfnee, pdfShifted_BXDF, _mis, _l);

    float jacobianPath = pathStateShifted.Jacobian; // Jacobian at vertex is 1

    //float mainPathPrefixPDF = exp(pathStateShifted.LogPathPdfMain);
    //float shiftedPathPrefixPDF = exp(pathStateShifted.LogPathPdfShifted);

    float mainPathPrefixPDF = pathStateShifted.LogPathPdfMain;
    float shiftedPathPrefixPDF = pathStateShifted.LogPathPdfShifted;

    float mainPDF_NEE = v1.PDF_NEE;
    float shiftPDF_NEE = v1.PDF_NEE; // Note: As vertex position is the same, NEE pdf is the same

    float mainPathPDF_NEE = mainPathPrefixPDF * mainPDF_NEE;
    float shiftedPathPDF_NEE = jacobianPath * shiftedPathPrefixPDF * shiftPDF_NEE;

	float mis_nee_s = 1.0f;
	float mis_nee_m = 1.0f;
    float mis_path = 1.0f;
    if (pathStateShifted.IsUsingNeeMis)
    {
        float mainPDF_BXDF = v1.PDF_BXDF_at_NEE;
        float shiftPDF_BXDF = pdfShifted_BXDF;

        float mainPathPDF_BXDF = mainPathPrefixPDF * mainPDF_BXDF;
        float shiftedPathPDF_BXDF = jacobianPath * shiftedPathPrefixPDF * shiftPDF_BXDF;

        // Selecting (Main, NEE) PDF
        //mis_path = BalanceHeuristic(
        //    mainPathPDF_NEE, mainPathPDF_BXDF, shiftedPathPDF_NEE, shiftedPathPDF_BXDF,
        //    gSettings.DirectNumSamples, 1, gSettings.DirectNumSamples, 1);

		mis_path = BalanceHeuristic(
            mainPathPrefixPDF, jacobianPath * shiftedPathPrefixPDF,
            gSettings.DirectNumSamples, gSettings.DirectNumSamples);

		mis_nee_m = BalanceHeuristic(
            mainPDF_NEE, mainPDF_BXDF,
            gSettings.DirectNumSamples, 1);

		mis_nee_s = BalanceHeuristic(
            shiftPDF_NEE, shiftPDF_BXDF,
            gSettings.DirectNumSamples, 1);

#ifdef GDPT_MIS_WEIGHT
        mis_path = GDPT_MIS_WEIGHT;
#endif
    }
    else
    {
        // Selecting (Main, NEE) PDF
        mis_path = BalanceHeuristic(
            mainPathPDF_NEE, shiftedPathPDF_NEE,
            gSettings.DirectNumSamples, gSettings.DirectNumSamples);
    }

    // Note: No NEE/BxDF MIS weights applied here
    float3 mainVertexLo = v1.ShadowFactor * v1.DirectContribution / max(1e-6f, mainPDF_NEE);
    float3 shiftedVertexLo = v1.ShadowFactor * radiance / max(1e-6f, mainPDF_NEE);

    float3 mainLo = mis_nee_m * v1.InitialBeta * mainVertexLo;
    float3 shiftedLo = mis_nee_s * jacobianPath * pathState.Beta * shiftedVertexLo;

    pathState.Lo = shiftedLo;

    float3 gradient = (shiftedLo - mainLo) * mis_path; // Flipped at the end if backwards estimator
    pathStateShifted.SubpathGradients += gradient;

    DBG_OUTPUT3(mainLo, GD_Nee_MainLo);
    DBG_OUTPUT3(shiftedLo, GD_Nee_ShiftLo);
    DBG_OUTPUT3(gradient, GD_Nee_Gradient);
    DBG_OUTPUT1(mis_path, GD_Nee_MIS);
    DBG_OUTPUT1(jacobianPath, GD_Nee_Jacobian);

    return true;
}

bool SampleDirect_S_Unconnected(
    inout PathState pathState,
    inout PathStateShifted pathStateShifted,

    PathVertexInfo v1,
    HitInfo hitInfo,
    float3 wo)
{
    // TODO: Multiple NEE samples

    float shadowFactor;
    float pdfShifted_NEE, pdfShifted_BXDF;
    float mis_nee;
    LightEval lightEval;
    float3 radiance = EvaluateNEE(hitInfo, gSettings.DirectNumSamples, wo, v1.LightSample, LobeTag::eAll, shadowFactor, pdfShifted_NEE, pdfShifted_BXDF, mis_nee, lightEval);

    if ((v1.PDF_NEE > 0.0f) != (pdfShifted_NEE > 0.0f))
    {
        pathStateShifted.IsSymmetric = false;
        DBG_OUTPUT1(1.0f, GD_RejectionNEE);
        return false;
    }

    if (v1.PDF_NEE <= 0.0f)
        return true;

    bool v1Dirac = MaterialIsDirac(v1.Roughness);
    bool isDirac = MaterialIsDirac(hitInfo.Mat.Roughness);
    if (!LightIsPunctual(v1.LightSample) && (v1Dirac || isDirac)) // TODO: This feels wrong
    {
        pathStateShifted.IsSymmetric = false;
        DBG_OUTPUT1(1.0f, GD_RejectionNEE);
        return false;
    }

    if (shadowFactor <= 0.0f)
    {
        pathStateShifted.IsSymmetric = false;
        DBG_OUTPUT1(1.0f, GD_RejectionNEE);
        return false;
    }

    float jacobianVertex = 1.0f;
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

        jacobianVertex = shiftResultEmissive.Jacobian;
    }

    float jacobianPrefix = pathStateShifted.Jacobian;
    float jacobianPath = jacobianPrefix * jacobianVertex;

    //float mainPathPrefixPDF = exp(pathStateShifted.LogPathPdfMain);
    //float shiftedPathPrefixPDF = exp(pathStateShifted.LogPathPdfShifted);

    float mainPathPrefixPDF = pathStateShifted.LogPathPdfMain;
    float shiftedPathPrefixPDF = pathStateShifted.LogPathPdfShifted;

    float mainPDF_NEE = v1.PDF_NEE;
    float shiftPDF_NEE = pdfShifted_NEE;

    float mainPathPDF_NEE = mainPathPrefixPDF * mainPDF_NEE;
    float shiftedPathPDF_NEE = jacobianPath * shiftedPathPrefixPDF * shiftPDF_NEE;

	float mis_nee_m = 1.0f;
	float mis_nee_s = 1.0f;
    float mis_path = 1.0f;
    if (pathStateShifted.IsUsingNeeMis)
    {
        float mainPDF_BXDF = v1.PDF_BXDF_at_NEE;
        float shiftPDF_BXDF = pdfShifted_BXDF;

        float mainPathPDF_BXDF = mainPathPrefixPDF * mainPDF_BXDF;
        float shiftedPathPDF_BXDF = jacobianPath * shiftedPathPrefixPDF * shiftPDF_BXDF;

        // Selecting (Main, NEE) PDF
        //mis_path = BalanceHeuristic(
        //    mainPathPDF_NEE, mainPathPDF_BXDF, shiftedPathPDF_NEE, shiftedPathPDF_BXDF,
        //    gSettings.DirectNumSamples, 1, gSettings.DirectNumSamples, 1);

		mis_path = BalanceHeuristic(
            mainPathPrefixPDF, jacobianPath * shiftedPathPrefixPDF,
            gSettings.DirectNumSamples, gSettings.DirectNumSamples);

		mis_nee_m = BalanceHeuristic(
            mainPDF_NEE, mainPDF_BXDF,
            gSettings.DirectNumSamples, 1);

		mis_nee_s = BalanceHeuristic(
            shiftPDF_NEE, shiftPDF_BXDF,
            gSettings.DirectNumSamples, 1);

#ifdef GDPT_MIS_WEIGHT
        mis_path = GDPT_MIS_WEIGHT;
#endif
    }
    else
    {
        // Selecting (Main, NEE) PDF
        mis_path = BalanceHeuristic(
            mainPathPDF_NEE, shiftedPathPDF_NEE,
            gSettings.DirectNumSamples, gSettings.DirectNumSamples);
    }

    // Note: No NEE/BxDF MIS weights applied here
    float3 mainVertexLo = v1.ShadowFactor * v1.DirectContribution / max(1e-6f, mainPDF_NEE);
    float3 shiftedVertexLo = shadowFactor * radiance / max(1e-6f, mainPDF_NEE);

    float3 mainLo = mis_nee_m * v1.InitialBeta * mainVertexLo;
    float3 shiftedLo = mis_nee_s * jacobianPath * pathState.Beta * shiftedVertexLo;

    pathState.Lo = shiftedLo;

    float3 gradient = (shiftedLo - mainLo) * mis_path; // Flipped at the end if backwards estimator
    pathStateShifted.SubpathGradients += gradient;

    DBG_OUTPUT3(mainLo, GD_Nee_MainLo);
    DBG_OUTPUT3(shiftedLo, GD_Nee_ShiftLo);
    DBG_OUTPUT3(gradient, GD_Nee_Gradient);
    DBG_OUTPUT1(mis_path, GD_Nee_MIS);
    DBG_OUTPUT1(jacobianPath, GD_Nee_Jacobian);
    return true;
}

bool SampleDirect_S(
    inout PathState pathState,
    inout PathStateShifted pathStateShifted,

    PathVertexInfo v1,
    HitInfo hitInfo,
    float3 wo)
{
    if (pathStateShifted.ReconnectionState == ReconnectionState::eUnconnected)
        return SampleDirect_S_Unconnected(pathState, pathStateShifted, v1, hitInfo, wo);

    if (pathStateShifted.ReconnectionState == ReconnectionState::eSemiConnected)
        return SampleDirect_S_Semiconnected(pathState, pathStateShifted, v1, hitInfo, wo);

    // TODO: Assert connected

    return SampleDirect_S_Connected(pathState, pathStateShifted, v1, hitInfo, wo);
}

#endif