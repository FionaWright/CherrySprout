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
    out float pdf)
{
    shadowFactor = 0.0f;
    pdf = 0.0f;

    LightEval lightEval;
    lightSample = SampleLight(rng, hitInfo.HitPos, wo, lightEval);

    lightDistance = lightEval.Distance;

    float NdL = dot(hitInfo.Ns_ff, lightEval.Direction);
    if (NdL < 0.0f || length(lightEval.Radiance) == 0.0f)
        return 0;

    TraceRayShadow(hitInfo.HitPosBiased, lightEval.Direction, lightEval.Distance, shadowFactor);
    if (shadowFactor <= 0.0f)
        return 0;

    float3 m;
    if (FEAT_DBG(FixedMisNeeManual))
    {
        m = GetFixedMisNee();
    }
    else if (lightEval.IsDelta)
    {
        m = 1.0f;
    }
    else
    {
        BxdfEval bxdfEval = BxDF::Evaluate(hitInfo, wo, lightEval.Direction, LobeTag::eAll);

        m = bxdfEval.Radiance * PowerHeuristic(lightEval.PDF, bxdfEval.PDF, numSamples, 1);
    }

    DBG_OUTPUT1(m, NEE_MIS);

    pdf = lightEval.PDF;
    return shadowFactor * lightEval.Radiance * m * NdL;
}

float3 SampleNEE(
    RngValuesAncillary::RngValuesNEE rng,
    HitInfo hitInfo,
    uint numSamples,
    float3 wo,

    out float pdf)
{
    LightSample _1;
    float _2;
    float _3;
    return SampleNEE(rng, hitInfo, numSamples, wo, _1, _2, _3, pdf);
}

float3 EvaluateNEE(
    HitInfo hitInfo,

    uint numSamples,
    float3 wo,
    LightSample lightSample,
    LobeTag lobe,

    out float shadowFactor,
    out float pdf)
{
    shadowFactor = 0.0f;
    pdf = 0.0f;

    LightEval lightEval = EvaluateLight(hitInfo.HitPos, wo, lightSample);

    float NdL = dot(hitInfo.Ns_ff, lightEval.Direction);
    if (NdL <= 0.0f)
        return 0;

    if (length(lightEval.Radiance) == 0.0f)
        return 0;

    TraceRayShadow(hitInfo.HitPosBiased, lightEval.Direction, lightEval.Distance, shadowFactor);
    if (shadowFactor <= 0.0f)
        return 0;

    float3 m;
    if (FEAT_DBG(FixedMisNeeManual))
    {
        m = GetFixedMisNee();
    }
    else if (lightEval.IsDelta)
    {
        m = 1.0f;
    }
    else
    {
        BxdfEval bxdfEval = BxDF::Evaluate(hitInfo, wo, lightEval.Direction, lobe);

        m = bxdfEval.Radiance * PowerHeuristic(lightEval.PDF, bxdfEval.PDF, numSamples, 1);
    }

    pdf = lightEval.PDF;
    return shadowFactor * lightEval.Radiance * m * NdL;
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
    out float pdf) { lightSample = (LightSample)0; shadowFactor = NAN; lightDistance = NAN; pdf = NAN; return NAN; }

float3 SampleNEE(
    RngValuesAncillary::RngValuesNEE rng,
    HitInfo hitInfo,

    uint numSamples,
    float3 wo,
    out float pdf) { pdf = NAN; return NAN; }

float3 EvaluateNEE(
    HitInfo hitInfo,

    uint numSamples,
    float3 wo,
    LightSample lightSample,
    LobeTag lobe,

    out float shadowFactor,
    out float pdf) { shadowFactor = NAN; pdf = NAN; return NAN; }

#endif

#endif
