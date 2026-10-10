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
    lightSample = SampleLight(rngInfo, hitInfo, pathState, bxdf, wo);

    float NdL = dot(hitInfo.Ns_ff, lightSample.Direction);
    if (length(lightSample.Radiance) == 0.0f || NdL < 0.0f)
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

        m = f_bxdf * PowerHeuristic(lightSample.PDF, pdf_bxdf, numSamples, 1);
    }

    DBG_OUTPUT1(m, NEE_MIS);

    return shadowFactor * lightSample.Radiance * m * NdL / lightSample.PDF;
}

float3 EvaluateNEE(
    HitInfo hitInfo,
    PathState pathState,
    BxDF bxdf,

    uint numSamples,
    float3 wo,
    float3 nextOrigin,

    uint lightIdx,
    float3 wi,

    out float pdf)
{
    LightSample lightSample = EvaluateLight(hitInfo, pathState, bxdf, lightIdx, wi, wo);

    pdf = lightSample.PDF;

    float NdL = dot(hitInfo.Ns_ff, lightSample.Direction);
    if (length(lightSample.Radiance) == 0.0f || NdL < 0.0f)
    {
        return 0;
    }

    float shadowFactor;
    TraceRayShadow(nextOrigin, lightSample.Direction, lightSample.Distance, shadowFactor);
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

        m = f_bxdf * PowerHeuristic(lightSample.PDF, pdf_bxdf, numSamples, 1);
    }

    return shadowFactor * lightSample.Radiance * m * NdL / lightSample.PDF;
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

    uint lightIdx,
    float3 wi,

    out float pdf) { pdf = NAN; return NAN; }

#endif

#endif
