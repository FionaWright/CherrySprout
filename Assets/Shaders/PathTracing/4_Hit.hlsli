#ifndef H_HIT_H
#define H_HIT_H

#include "PathTracing/Debug/OutputColorMacros.hlsli"

#include "PathTracing/HitInfo/GetHitInfo.hlsli"
#include "BxDFs/GetBxDF.hlsli"

#include "Utils/RandomDirection.h"

#include "PathTracing/5_SampleDirect.hlsli"
#include "PathTracing/5_SampleIndirect.hlsli"

void Hit(HitInfo hitInfo, inout PathState pathState, inout float3 L_sample, inout RngInfo rngInfo, uint2 pixelCoord)
{
    float3 hitPos = pathState.Desc.Origin + pathState.Desc.Direction * hitInfo.RayT;
    float3 nextOrigin = hitPos + hitInfo.Ng_ff * EPSILON;

    if (FEATURE_ENABLED(Transient))
        pathState.RollingPathDistance += hitInfo.RayT;

    L_sample = pathState.Beta * hitInfo.Emission;

    float3 wo = -pathState.Desc.Direction;

    BxDF bxdf;

    if (FEATURE_ENABLED(NEE))
    {
        float3 E_direct = 0;
        for (uint i = 0; i < gSettings.DirectNumSamples; i++)
        {
            float pdf;
            E_direct += SampleDirectLighting(rngInfo, hitInfo, pathState, bxdf, pixelCoord, wo, hitPos, nextOrigin, pdf);

            if (FEATURE_ENABLED(GradientDomain))
                pathState.PDF += pdf;
        }

        L_sample += E_direct * pathState.Beta / float(gSettings.DirectNumSamples);
    }

    float3 wi;
    float pdf;
    float3 E_indirect = SampleIndirectLighting(rngInfo, hitInfo, bxdf, pathState, wo, wi, pdf);
    pathState.Beta *= E_indirect;

    pathState.LastBxdfPdf = pdf;

    if (FEATURE_ENABLED(GradientDomain))
        pathState.PDF *= pdf;

    pathState.Desc.Direction = wi;
    pathState.Desc.Origin = nextOrigin;
}

#endif