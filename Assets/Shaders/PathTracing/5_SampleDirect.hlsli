#ifndef H_SAMPLE_DIRECT_H
#define H_SAMPLE_DIRECT_H

#include "PathTracing/NEE/SampleNEE.hlsli"

#include "PathTracing/ReSTIR/DI.hlsli"
#include "PathTracing/Transient.hlsli"
#include "PathTracing/RandomSample.hlsli"

float3 SampleDirectLighting(
    RngValuesAncillary::RngValuesNEE rng,
    HitInfo hitInfo,
    PathState pathState,

    uint2 pixelCoord,
    float3 wo,

    inout PathVertexInfo vInfo)
{
    float3 E_direct = 0;

    LightSample lightSample = INIT0(LightSample);
    float shadowFactor = 0.0f;

    float3 radiance = 0.0f;
    float lightDistance = INF;
    float pdf_nee = 1.0f;

    if (FEAT_CORE(RestirDI) && pathState.IsPrimaryRay) // TODO: PDF
    {
        CandidateDI lightSE;
        bool validReservoir = TrySampleReservoirDI(hitInfo, pixelCoord, wo, gSettings.DirectNumSamples, lightSE, lightDistance, radiance);

        DBG_OUTPUT1(validReservoir, RESTIRDI_ValidReservoir);

        E_direct += radiance;
        lightSample = lightSE.LightSample;
    }
    else
    {
        radiance = SampleNEE(rng, hitInfo, gSettings.DirectNumSamples, wo, lightSample, lightDistance, shadowFactor, pdf_nee);

        if (FEAT_DBG(NeeTestRevaluate))
        {
            radiance = EvaluateNEE(hitInfo, gSettings.DirectNumSamples, wo, lightSample, LobeTag::eAll, shadowFactor, pdf_nee);
        }

        E_direct += radiance / max(1e-6f, pdf_nee);

        DBG_OUTPUT3(radiance / max(1e-6f, pdf_nee), NEE_Contrib);
        DBG_OUTPUT1(shadowFactor, NEE_ShadowFactor);
    }

    // TODO: Move shadow factor outside?

	if (FEAT_CORE(GradientDomain) || FEAT_CORE(RestirPT))
    {
	    vInfo.LightSample = lightSample;
	    vInfo.ShadowFactor = shadowFactor;
        vInfo.DirectContribution = radiance;
        vInfo.PDF_NEE = pdf_nee;
    }

    if (FEAT_CORE(Transient))
    {
        if (gSettings.TransientLightIdx == -1 || gSettings.TransientLightIdx == lightSample.Index)
        {
            float transientFactor = GetTransientFactor(pathState.RollingPathDistance + lightDistance);
            E_direct *= transientFactor;
        }
        else
            return 0;
    }

    return E_direct;
}

#endif