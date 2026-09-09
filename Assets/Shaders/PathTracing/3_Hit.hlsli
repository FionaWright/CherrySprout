#ifndef H_HIT_H
#define H_HIT_H

#include "PathTracing/Debug/OutputColorMacros.hlsli"

#include "PathTracing/HitInfo/GetHitInfo.hlsli"
#include "BxDFs/GetBxDF.hlsli"

#include "Utils/RandomDirection.h"

#include "PathTracing/4_SampleDirect.hlsli"
#include "PathTracing/4_SampleIndirect.hlsli"

void Hit(HitInfo hitInfo, inout PathState pathState, inout float3 L_sample, inout RngInfo rngInfo, uint2 pixelCoord)
{
    float3 hitPos = pathState.Desc.Origin + pathState.Desc.Direction * hitInfo.RayT;
    float3 nextOrigin = hitPos + hitInfo.Ng_ff * EPSILON;

#if FEATURE_ENABLED(Transient)
    pathState.RollingPathDistance += hitInfo.RayT;
#endif

    L_sample = pathState.Beta * hitInfo.Emission;

    float3 wo = -pathState.Desc.Direction;

    // TODO: Perform average luminance tests between with/without NEE. Should be equal. Set up python executor

    BxDF bxdf;

    if (FEATURE_ENABLED(NEE))
    {
        float3 E_direct = SampleDirectLighting(rngInfo, hitInfo, pathState, bxdf, pixelCoord, wo, hitPos, nextOrigin);
        L_sample += E_direct * pathState.Beta;
    }

    float3 wi;
    float pdf;
    float3 E_indirect = SampleIndirectLighting(rngInfo, hitInfo, bxdf, pathState, wo, wi, pdf);
    pathState.Beta *= E_indirect;

    pathState.LastBxdfPdf = pdf;

    pathState.Desc.Direction = wi;
    pathState.Desc.Origin = nextOrigin;
}

#endif