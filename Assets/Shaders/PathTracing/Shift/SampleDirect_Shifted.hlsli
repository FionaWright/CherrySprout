#ifndef H_NEE_SHIFTED_UNCONNECTED_H
#define H_NEE_SHIFTED_UNCONNECTED_H

#include "PathTracing/Shift/ShiftMapping/Components/Reconnect.hlsli"

bool SampleDirect_S_Connected(
    inout PathState pathState,
    inout PathStateShifted pathStateShifted,

    PathVertexInfo v1,
    HitInfo hitInfo,
    float3 wo)
{
    if (v1.Lobe == LobeTag::eNEE)
    {
        float3 L_sample = v1.ShadowFactor * v1.DirectContribution * pathState.Beta / max(1e-6f, v1.PDF_NEE);
        pathState.Lo += ApplyFireflyThreshold(L_sample, gSettings.FireflyThreshold);
        return false;
    }

    float3 L_sample = v1.ShadowFactor * v1.MIS * v1.DirectContribution * pathState.Beta / max(1e-6f, v1.PDF_NEE);
    pathState.Lo += ApplyFireflyThreshold(L_sample, gSettings.FireflyThreshold);
    return true;
}

bool SampleDirect_S_Semiconnected(
    inout PathState pathState,
    inout PathStateShifted pathStateShifted,

    PathVertexInfo v1,
    HitInfo hitInfo,
    float3 wo)
{
    if (v1.Lobe == LobeTag::eNEE) // TODO: Not accounting for different bxdf eval
    {
        float3 L_sample = v1.ShadowFactor * v1.DirectContribution * pathState.Beta / max(1e-6f, v1.PDF_NEE);
        pathState.Lo += ApplyFireflyThreshold(L_sample, gSettings.FireflyThreshold);
        return false;
    }

    float shadowFactor; // TODO: Is this necessary? (Only need PDF) (Not accounting for emissives)
    float pdf_nee;
    float mis_nee; // TODO: Use v1.PDF_NEE to recompute MIS?
    LightEval lightEval;
    float3 radiance = EvaluateNEE(hitInfo, gSettings.DirectNumSamples, wo, v1.LightSample, LobeTag::eAll, shadowFactor, pdf_nee, mis_nee, lightEval);

    if ((v1.PDF_NEE > 0.0f) != (pdf_nee > 0.0f))
    {
        pathStateShifted.IsSymmetric = false;
        DBG_OUTPUT1(1.0f, GD_RejectionNEE);
        return false;
    }

    //BxdfEval bxdfEval_NEE = BxDF::Evaluate(hitInfo, wo, lightEval.Direction, LobeTag::eAll);
    //mis_nee = PowerHeuristic(v1.PDF_NEE, bxdfEval_NEE.PDF, gSettings.DirectNumSamples, 1);

    //float3 L_sample = shadowFactor * mis_nee * radiance * pathState.Beta / max(1e-6f, pdf_nee);
    float3 L_sample = shadowFactor * mis_nee * radiance * pathState.Beta / max(1e-6f, v1.PDF_NEE); // TODO
    pathState.Lo += ApplyFireflyThreshold(L_sample, gSettings.FireflyThreshold);
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

    // TODO: NEE Lobe tag section not updated

    float shadowFactor;
    float pdf_nee;
    float mis_nee;
    LightEval lightEval;
    float3 radiance = EvaluateNEE(hitInfo, gSettings.DirectNumSamples, wo, v1.LightSample, LobeTag::eAll, shadowFactor, pdf_nee, mis_nee, lightEval);

    if ((v1.PDF_NEE > 0.0f) != (pdf_nee > 0.0f))
    {
        pathStateShifted.IsSymmetric = false;
        DBG_OUTPUT1(1.0f, GD_RejectionNEE);
        return false;
    }

    if (v1.Lobe == LobeTag::eNEE) // TODO: Outdated
    {
        // TODO: ?
        //float3 E_direct = v1.MIS * radiance / max(1e-6f, v1.PDF_NEE);
        float3 E_direct = radiance / max(1e-6f, v1.PDF_NEE);
        DBG_OUTPUT3(E_direct, GD_NeeContrib);

        pathStateShifted.PdfRatio *= (pdf_nee / max(1e-6f, v1.PDF_NEE));

        float3 L_sample = E_direct * pathState.Beta;
        pathState.Lo += ApplyFireflyThreshold(L_sample, gSettings.FireflyThreshold);
        return false;
    }

    if (v1.PDF_NEE <= 0.0f)
        return true;

    if (shadowFactor <= 0.0f)
    {
        pathStateShifted.IsSymmetric = false;
        DBG_OUTPUT1(1.0f, GD_RejectionNEE);
        return false;
    }

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

        pdf_nee /= shiftResultEmissive.Jacobian;

        // TODO: Slow
        //BxdfEval bxdfEval = BxDF::Evaluate(hitInfo, wo, lightEval.Direction, LobeTag::eAll);
        //mis_nee = PowerHeuristic(pdf_nee, bxdfEval.PDF, gSettings.DirectNumSamples, 1);
    }

    // TODO: Slow
    BxdfEval bxdfEval = BxDF::Evaluate(hitInfo, wo, lightEval.Direction, LobeTag::eAll);
    mis_nee = PowerHeuristic(v1.PDF_NEE, bxdfEval.PDF, gSettings.DirectNumSamples, 1);

    //float3 E_direct = shadowFactor * mis_nee * radiance / max(1e-6f, pdf_nee);
    float3 E_direct = shadowFactor * mis_nee * radiance / max(1e-6f, v1.PDF_NEE);
    DBG_OUTPUT3(E_direct, GD_NeeContrib);

    float3 L_sample = E_direct * pathState.Beta;
    pathState.Lo += ApplyFireflyThreshold(L_sample, gSettings.FireflyThreshold);

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