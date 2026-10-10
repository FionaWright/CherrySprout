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
    float3 nextOrigin,

    inout PathVertexInfo currentVertexInfo)
{
    float3 E_direct = 0;

    LightSample lightSample = (LightSample)0;

    if (FEAT_CORE(RestirDI) && pathState.RaySegmentIdx == 0)
    {
        E_direct += SampleReservoir(hitInfo, pixelCoord, wo, nextOrigin, bxdf, gSettings.DirectNumSamples, lightSample);
    }
    else
    {
        float3 L_nee = SampleNEE(rngInfo, hitInfo, pathState, bxdf, gSettings.DirectNumSamples, wo, nextOrigin, lightSample);

        if (FEAT_DBG(NeeTestRevaluate))
        {
            float _;
            L_nee = EvaluateNEE(hitInfo, pathState, bxdf, gSettings.DirectNumSamples, wo, nextOrigin, lightSample.Index, lightSample.Direction, _);
        }

        E_direct += L_nee;

        DBG_OUTPUT3(L_nee, NEE_Contrib);
    }

	if (FEAT_CORE(GradientDomain))
	{
        // TODO
		//currentVertexInfo.PDF *= lightSample.PDF;
    	currentVertexInfo.NeeLightIdx = lightSample.Index;
    	currentVertexInfo.NeeLightDirection = lightSample.Direction;
	}

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