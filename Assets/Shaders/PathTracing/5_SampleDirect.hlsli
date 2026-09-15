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
    float3 hitPos,
    float3 nextOrigin,

    out float pdf)
{
    float3 E_direct = 0;

    LightSample lightSample = (LightSample)0;

    pdf = NAN; // TODO

    if (FEATURE_ENABLED(RestirDI) && pathState.RaySegmentIdx == 0)
    {
        E_direct += SampleReservoir(hitInfo, pixelCoord, wo, nextOrigin, bxdf, gSettings.DirectNumSamples, lightSample);
    }
    else
    {
        E_direct += SampleNEE(rngInfo, hitInfo, pathState, bxdf, gSettings.DirectNumSamples, wo, hitPos, nextOrigin, lightSample);
    }

    if (FEATURE_ENABLED(Transient))
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