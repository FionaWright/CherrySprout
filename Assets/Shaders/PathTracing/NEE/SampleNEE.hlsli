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
    out float shadowFactor)
{
    BxDF bxdf;

    LightEval lightEval;
    lightSample = SampleLight(rng, hitInfo.HitPos, bxdf, wo, lightEval);

    lightDistance = lightEval.Distance;

    shadowFactor = 0.0f;

    float NdL = dot(hitInfo.Ns_ff, lightEval.Direction);
    if (NdL < 0.0f || length(lightEval.Radiance) == 0.0f)
        return 0;

    TraceRayShadow(hitInfo.HitPosBiased, lightEval.Direction, lightEval.Distance, shadowFactor);
    if (shadowFactor <= 0.0f)
        return 0;

    float3 m;
    if (FEAT_DBG(FixedMisNeeManual))
    {
        m = gDebugSettings.FixedMisNee;
    }
    else if (lightEval.IsDelta)
    {
        m = 1.0f;
    }
    else
    {
        BxdfEval bxdfEval = bxdf.Evaluate(hitInfo, wo, lightEval.Direction, LobeTag::eAll);

        m = bxdfEval.Radiance * PowerHeuristic(lightEval.PDF, bxdfEval.PDF, numSamples, 1);
    }

    DBG_OUTPUT1(m, NEE_MIS);

    return shadowFactor * lightEval.Radiance * m * NdL / max(1e-6f, lightEval.PDF);
}

float3 SampleNEE(
    RngValuesAncillary::RngValuesNEE rng,
    HitInfo hitInfo,

    uint numSamples,
    float3 wo)
{
    LightSample _1;
    float _2;
    float _3;
    return SampleNEE(rng, hitInfo, numSamples, wo, _1, _2, _3);
}

float3 EvaluateNEE(
    HitInfo hitInfo,

    uint numSamples,
    float3 wo,
    LightSample lightSample,

    out float shadowFactor)
{
    shadowFactor = 0.0f;

    BxDF bxdf;
    LightEval lightEval = EvaluateLight(hitInfo.HitPos, bxdf, wo, lightSample);

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
        m = gDebugSettings.FixedMisNee;
    }
    else if (lightEval.IsDelta)
    {
        m = 1.0f;
    }
    else
    {
        BxdfEval bxdfEval = bxdf.Evaluate(hitInfo, wo, lightEval.Direction, LobeTag::eAll);

        m = bxdfEval.Radiance * PowerHeuristic(lightEval.PDF, bxdfEval.PDF, numSamples, 1);
    }

    return shadowFactor * lightEval.Radiance * m * NdL / lightEval.PDF;
}

#else

float3 SampleNEE(
    RngValuesAncillary::RngValuesNEE rng,
    HitInfo hitInfo,

    uint numSamples,
    float3 wo,

    out LightSample lightSample,
    out float lightDistance,
    out float shadowFactor) { lightSample = (LightSample)0; shadowFactor = NAN; lightDistance = NAN; return NAN; }

float3 SampleNEE(
    RngValuesAncillary::RngValuesNEE rng,
    HitInfo hitInfo,

    uint numSamples,
    float3 wo) { return NAN; }

float3 EvaluateNEE(
    HitInfo hitInfo,

    uint numSamples,
    float3 wo,
    LightSample lightSample,

    out float shadowFactor) { shadowFactor = NAN; return NAN; }

#endif

#endif
