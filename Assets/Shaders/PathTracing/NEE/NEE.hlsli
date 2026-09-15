#ifndef H_NEE_H
#define H_NEE_H

#if FEATURE_ENABLED_PP(NEE)

#include "PathTracing/NEE/SampleLight.hlsli"

float3 SampleNEE(
    inout RngInfo rngInfo,
    HitInfo hitInfo,
    PathState pathState,
    BxDF bxdf,

    uint numSamples,
    float3 wo,
    float3 hitPos,
    float3 nextOrigin,

    out LightSample lightSample)
{
    float3 E_direct = 0;

    lightSample = SampleLight(rngInfo, hitInfo, pathState, bxdf, wo, hitPos, nextOrigin);

    float NdL = dot(hitInfo.Ns_ff, lightSample.Direction);
    if (length(lightSample.Radiance) == 0.0f || NdL < 0.0f)
    {
        return 0;
    }

    float shadowFactor;
    TraceRayShadow(nextOrigin, lightSample.Direction, lightSample.Distance, shadowFactor);
    DBG_OUTPUT1(shadowFactor,     NEE_Occluded);

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

    DBG_OUTPUT1(m,                       NEE_MIS_Weight);
    DBG_OUTPUT3(lightSample.Radiance,    NEE_Radiance);

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
    float3 hitPos,
    float3 nextOrigin,

    out LightSample lightSample) { return NAN; }

#endif

#endif
