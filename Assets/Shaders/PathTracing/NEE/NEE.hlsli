#ifndef H_NEE_H
#define H_NEE_H

#if FEAT_CORE_D(NEE)

#include "PathTracing/NEE/SampleLight.hlsli"

float3 SampleNEE(
    inout RngInfo rngInfo,
    HitInfo hitInfo,
    PathState pathState,
    BxDF bxdf,

    uint numSamples,
    float3 wo,
    float3 nextOrigin,

    out LightSample lightSample)
{
    LightEval lightEval;
    lightSample = SampleLight(rngInfo, hitInfo, pathState, bxdf, wo, lightEval);

    float NdL = dot(hitInfo.Ns_ff, lightSample.Direction);
    if (length(lightEval.Radiance) == 0.0f || NdL < 0.0f)
    {
        DBG_OUTPUT1(0,     NEE_ShadowFactor);
        return 0;
    }

    float shadowFactor;
    TraceRayShadow(hitInfo.HitPos, lightSample.Direction, lightSample.Distance, shadowFactor); // TODO: Only works when using hitPos NOT nextOrigin, why?
    DBG_OUTPUT1(shadowFactor,     NEE_ShadowFactor);

    if (shadowFactor <= 0.0f)
        return 0;

    float3 m;
    if (lightSample.IsDelta)
    {
        m = 1.0f;
    }
    else
    {
        float3 f_bxdf;
        float pdf_bxdf;
        bxdf.Evaluate(hitInfo, wo, lightSample.Direction, f_bxdf, pdf_bxdf);

        m = f_bxdf * PowerHeuristic(lightEval.PDF, pdf_bxdf, numSamples, 1);
    }

    DBG_OUTPUT1(m, NEE_MIS);

    return shadowFactor * lightEval.Radiance * m * NdL / max(1e-6f, lightEval.PDF);
}

float3 EvaluateNEE(
    HitInfo hitInfo,
    PathState pathState,
    BxDF bxdf,

    uint numSamples,
    float3 wo,
    float3 nextOrigin,

    LightSample lightSample)
{
    float NdL = dot(hitInfo.Ns_ff, lightSample.Direction);
    if (NdL <= 0.0f)
        return 0;

    LightEval lightEval = EvaluateLight(hitInfo, pathState, bxdf, wo, lightSample);

    if (length(lightEval.Radiance) == 0.0f)
        return 0;

    float shadowFactor;
    TraceRayShadow(hitInfo.HitPos, lightSample.Direction, lightSample.Distance, shadowFactor);
    if (shadowFactor <= 0.0f)
        return 0;

    float3 m;
    if (lightSample.IsDelta)
    {
        m = 1.0f;
    }
    else
    {
        float3 f_bxdf;
        float pdf_bxdf;
        bxdf.Evaluate(hitInfo, wo, lightSample.Direction, f_bxdf, pdf_bxdf);

        m = f_bxdf * PowerHeuristic(lightEval.PDF, pdf_bxdf, numSamples, 1);
    }

    return shadowFactor * lightEval.Radiance * m * NdL / lightEval.PDF;
}

#else

float3 SampleNEE(
    inout RngInfo rngInfo,
    HitInfo hitInfo,
    PathState pathState,
    BxDF bxdf,

    uint numSamples,
    float3 wo,
    float3 nextOrigin,

    out LightSample lightSample) { return NAN; }

float3 EvaluateNEE(
    HitInfo hitInfo,
    PathState pathState,
    BxDF bxdf,

    uint numSamples,
    float3 wo,
    float3 nextOrigin,

    LightSample lightSample) { pdf = NAN; return NAN; }

#endif

#endif
