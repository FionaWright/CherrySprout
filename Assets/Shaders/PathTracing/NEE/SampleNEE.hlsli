#ifndef H_NEE_H
#define H_NEE_H

#if FEAT_CORE_D(NEE)

#include "PathTracing/NEE/SampleLight.hlsli"

float3 SampleNEE(
    RngValuesAncillary::RngValuesNEE rng,
    HitInfo hitInfo,

    uint numSamples,
    float3 wo,

    out LightSample lightSample,
    out float lightDistance,
    out float shadowFactor,
    out float pdf_nee,
    out float pdf_bxdf,
    out float mis)
{
    shadowFactor = 0.0f;
    pdf_nee = 0.0f;
    pdf_bxdf = 0.0f;
    mis = 0.0f;

    LightEval lightEval;
    lightSample = SampleLight(rng, hitInfo.HitPos, lightEval);

    lightDistance = lightEval.Distance;

    float NdL = dot(hitInfo.Ns_ff, lightEval.Direction);
    if (NdL < 0.0f || length(lightEval.Radiance) == 0.0f)
        return 0;

    TraceRayShadow(hitInfo.HitPosBiased, lightEval.Direction, lightEval.Distance, shadowFactor);
    if (shadowFactor <= 0.0f)
        return 0;

    BxdfEval bxdfEval = BxDF::Evaluate(hitInfo, wo, lightEval.Direction, LobeTag::eAll);

    float3 radiance = lightEval.Radiance * bxdfEval.Radiance; // TODO: Did this introduce jacobian issues to punctuals?

    if (FEAT_DBG(FixedMisNeeManual))
    {
        mis = GetFixedMisNee();
    }
    else if (lightEval.IsDelta)
    {
        mis = 1.0f;
    }
    else
    {
        mis = PowerHeuristic(lightEval.PDF, bxdfEval.PDF, numSamples, 1);
    }

    DBG_OUTPUT1(mis, NEE_MIS);

    pdf_nee = lightEval.PDF;
    pdf_bxdf = bxdfEval.PDF;
    return shadowFactor * radiance * NdL;
}

float3 SampleNEE(
    RngValuesAncillary::RngValuesNEE rng,
    HitInfo hitInfo,
    uint numSamples,
    float3 wo,

    out float pdf_nee,
    out float pdf_bxdf,
    out float mis)
{
    LightSample _1;
    float _2;
    float _3;
    return SampleNEE(rng, hitInfo, numSamples, wo, _1, _2, _3, pdf_nee, pdf_bxdf, mis);
}

float3 EvaluateNEE(
    HitInfo hitInfo,

    uint numSamples,
    float3 wo,
    LightSample lightSample,
    LobeTag lobe,

    out float shadowFactor,
    out float pdf_nee,
    out float pdf_bxdf,
    out float mis,
    out LightEval lightEval)
{
    shadowFactor = 0.0f;
    pdf_nee = 0.0f;
    pdf_bxdf = 0.0f;
    mis = 0.0f;

    lightEval = EvaluateLight(hitInfo.HitPos, lightSample);

    float NdL = dot(hitInfo.Ns_ff, lightEval.Direction);
    if (NdL <= 0.0f)
        return 0;

    if (length(lightEval.Radiance) == 0.0f)
        return 0;

    TraceRayShadow(hitInfo.HitPosBiased, lightEval.Direction, lightEval.Distance, shadowFactor);
    if (shadowFactor <= 0.0f)
        return 0;

    BxdfEval bxdfEval = BxDF::Evaluate(hitInfo, wo, lightEval.Direction, LobeTag::eAll);

    float3 radiance = lightEval.Radiance * bxdfEval.Radiance;

    if (FEAT_DBG(FixedMisNeeManual))
    {
        mis = GetFixedMisNee();
    }
    else if (lightEval.IsDelta)
    {
        mis = 1.0f;
    }
    else
    {
        mis = PowerHeuristic(lightEval.PDF, bxdfEval.PDF, numSamples, 1);
    }

    pdf_nee = lightEval.PDF;
    pdf_bxdf = bxdfEval.PDF;
    return shadowFactor * radiance * NdL;
}

float3 EvaluateNEE(
    HitInfo hitInfo,

    uint numSamples,
    float3 wo,
    LightSample lightSample,
    LobeTag lobe,

    out float shadowFactor,
    out float pdf_nee,
    out float pdf_bxdf,
    out float mis)
{
    LightEval _1;
    return EvaluateNEE(hitInfo, numSamples, wo, lightSample, lobe, shadowFactor, pdf_nee, pdf_bxdf, mis, _1);
}

#else

float3 SampleNEE(
    RngValuesAncillary::RngValuesNEE rng,
    HitInfo hitInfo,

    uint numSamples,
    float3 wo,

    out LightSample lightSample,
    out float lightDistance,
    out float shadowFactor,
    out float pdf_nee,
    out float pdf_bxdf,
    out float mis) { lightSample = (LightSample)0; shadowFactor = NAN; lightDistance = NAN; pdf_nee = NAN; pdf_bxdf = NAN; mis = NAN; return NAN; }

float3 SampleNEE(
    RngValuesAncillary::RngValuesNEE rng,
    HitInfo hitInfo,

    uint numSamples,
    float3 wo,
    out float pdf_nee,
    out float pdf_bxdf,
    out float mis) { pdf_nee = NAN; pdf_bxdf = NAN; mis = NAN; return NAN; }

float3 EvaluateNEE(
    HitInfo hitInfo,

    uint numSamples,
    float3 wo,
    LightSample lightSample,
    LobeTag lobe,

    out float shadowFactor,
    out float pdf_nee,
    out float pdf_bxdf,
    out float mis) { shadowFactor = NAN; pdf_nee = NAN; pdf_bxdf = NAN; mis = NAN; return NAN; }

float3 EvaluateNEE(
    HitInfo hitInfo,

    uint numSamples,
    float3 wo,
    LightSample lightSample,
    LobeTag lobe,

    out float shadowFactor,
    out float pdf_nee,
    out float pdf_bxdf,
    out float mis,
    out LightEval lightEval) { shadowFactor = NAN; pdf_nee = NAN; pdf_bxdf = NAN; mis = NAN; lightEval = INIT0(LightEval); return NAN; }

#endif

#endif
