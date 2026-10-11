#ifndef H_SAMPLE_DIRECT_H
#define H_SAMPLE_DIRECT_H

#include "PathTracing/NEE/NEE.hlsli"

#include "PathTracing/ReSTIR/ReSTIR_DI.hlsli"
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

    if (FEAT_CORE(RestirDI) && pathState.RaySegmentIdx == 0)
    {
        E_direct += SampleReservoir(hitInfo, pixelCoord, wo, bxdf, gSettings.DirectNumSamples, lightSample);
    }
    else
    {
        float3 L_nee = SampleNEE(rngInfo, hitInfo, pathState, bxdf, gSettings.DirectNumSamples, wo, lightSample);

        if (FEAT_DBG(NeeTestRevaluate))
        {
            L_nee = EvaluateNEE(hitInfo, pathState, bxdf, gSettings.DirectNumSamples, wo, lightSample);
        }

        E_direct += L_nee;

        DBG_OUTPUT3(L_nee, NEE_Contrib);
    }

	if (FEAT_CORE(GradientDomain))
	    currentVertexInfo.LightSample = lightSample;

    if (FEAT_CORE(Transient))
    {
        if (gSettings.TransientLightIdx == -1 || gSettings.TransientLightIdx == lightSample.Index)
        {
            float transientFactor = GetTransientFactor(pathState.RollingPathDistance + lightSample.Distance);
            E_direct *= transientFactor;
        }
        else
            return 0;
    }

    return E_direct;
}

#endif