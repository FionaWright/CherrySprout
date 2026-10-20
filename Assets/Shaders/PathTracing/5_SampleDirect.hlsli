#ifndef H_SAMPLE_DIRECT_H
#define H_SAMPLE_DIRECT_H

#include "PathTracing/NEE/NEE.hlsli"

#include "PathTracing/ReSTIR/DI.hlsli"
#include "PathTracing/Transient.hlsli"

float3 SampleDirectLighting(
    inout RngInfo rngInfo,
    HitInfo hitInfo,
    PathState pathState,
    BxDF bxdf,

    uint2 pixelCoord,
    float3 wo,

    inout PathVertexInfo currentVertexInfo)
{
    float3 E_direct = 0;

    LightSample lightSample = (LightSample)0;
    float shadowFactor = 0.0f;

    float lightDistance = INF;

    if (FEAT_CORE(RestirDI) && pathState.RaySegmentIdx == 0)
    {
        CandidateDI lightSE;
        float3 L_restir;
        bool validReservoir = TrySampleReservoir(hitInfo, pixelCoord, wo, bxdf, gSettings.DirectNumSamples, lightSE, lightDistance, L_restir);

        DBG_OUTPUT1(validReservoir, RESTIR_ValidReservoir);

        E_direct += L_restir;
        lightSample = lightSE.LightSample;
    }
    else
    {
        float3 L_nee = SampleNEE(rngInfo, hitInfo, bxdf, gSettings.DirectNumSamples, wo, lightSample, lightDistance, shadowFactor);

        if (FEAT_DBG(NeeTestRevaluate))
        {
            L_nee = EvaluateNEE(hitInfo, bxdf, gSettings.DirectNumSamples, wo, lightSample, shadowFactor);
        }

        E_direct += L_nee;

        DBG_OUTPUT3(L_nee, NEE_Contrib);
        DBG_OUTPUT1(shadowFactor, NEE_ShadowFactor);
    }

    // TODO: Move shadow factor outside?

	if (FEAT_CORE(GradientDomain))
    {
	    currentVertexInfo.LightSample = lightSample;
	    currentVertexInfo.ShadowFactor = shadowFactor;
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