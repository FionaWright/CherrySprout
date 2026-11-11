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
    out float pdf,
    out float mis)
{
    shadowFactor = 0.0f;
    pdf = 0.0f;
    mis = 0.0f;

    LightEval lightEval;
    lightSample = SampleLight(rng, hitInfo.HitPos, wo, lightEval);

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

    pdf = lightEval.PDF;
    return shadowFactor * radiance * NdL;
}

float3 SampleNEE(
    RngValuesAncillary::RngValuesNEE rng,
    HitInfo hitInfo,
    uint numSamples,
    float3 wo,

    out float pdf,
    out float mis)
{
    LightSample _1;
    float _2;
    float _3;
    return SampleNEE(rng, hitInfo, numSamples, wo, _1, _2, _3, pdf, mis);
}

float3 EvaluateNEE(
    HitInfo hitInfo,

    uint numSamples,
    float3 wo,
    LightSample lightSample,
    LobeTag lobe,

    out float shadowFactor,
    out float pdf,
    out float mis,
    out LightEval lightEval)
{
    shadowFactor = 0.0f;
    pdf = 0.0f;
    mis = 0.0f;

    lightEval = EvaluateLight(hitInfo.HitPos, wo, lightSample);

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

    pdf = lightEval.PDF;
    return shadowFactor * radiance * NdL;
}

float3 EvaluateNEE(
    HitInfo hitInfo,

    uint numSamples,
    float3 wo,
    LightSample lightSample,
    LobeTag lobe,

    out float shadowFactor,
    out float pdf,
    out float mis)
{
    LightEval _1;
    return EvaluateNEE(hitInfo, numSamples, wo, lightSample, lobe, shadowFactor, pdf, mis, _1);
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
    out float pdf,
    out float mis) { lightSample = (LightSample)0; shadowFactor = NAN; lightDistance = NAN; pdf = NAN; mis = NAN; return NAN; }

float3 SampleNEE(
    RngValuesAncillary::RngValuesNEE rng,
    HitInfo hitInfo,

    uint numSamples,
    float3 wo,
    out float pdf,
    out float mis) { pdf = NAN; mis = NAN; return NAN; }

float3 EvaluateNEE(
    HitInfo hitInfo,

    uint numSamples,
    float3 wo,
    LightSample lightSample,
    LobeTag lobe,

    out float shadowFactor,
    out float pdf,
    out float mis) { shadowFactor = NAN; pdf = NAN; mis = NAN; return NAN; }

float3 EvaluateNEE(
    HitInfo hitInfo,

    uint numSamples,
    float3 wo,
    LightSample lightSample,
    LobeTag lobe,

    out float shadowFactor,
    out float pdf,
    out float mis,
    out LightEval lightEval) { shadowFactor = NAN; pdf = NAN; mis = NAN; lightEval = INIT0(LightEval); return NAN; }

#endif

#endif
