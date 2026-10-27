#ifndef H_HIT_H
#define H_HIT_H

#include "PathTracing/Debug/Internal/OutputColorMacros.hlsli"
#include "PathTracing/Debug/Scales.hlsli"

#include "PathTracing/HitInfo/GetHitInfo.hlsli"
#include "BxDFs/GetBxDF.hlsli"

#include "PathTracing/RandomSample.hlsli"
#include "Utils/RandomDirection.h"

#include "PathTracing/5_SampleDirect.hlsli"
#include "PathTracing/5_SampleIndirect.hlsli"
#include "PathTracing/EvaluateHitEmission.hlsli"

void Hit(inout PathState pathState,
        RngValuesAncillary rng,

        HitInfo hitInfo,
        uint2 pixelCoord,

        out float3 L_sample,
        inout PathVertexInfo vInfo)
{
    L_sample = 0.0f;

    float3 wo = -pathState.Desc.Direction;

    float3 E_direct = 0;
    if (FEAT_CORE(NEE))
    {
        for (uint i = 0; i < gSettings.DirectNumSamples; i++)
        {
            E_direct += SampleDirectLighting(rng.NEE[i], hitInfo, pathState, pixelCoord, wo, vInfo);
        }

        DBG_SCALE_INTENSITY_DIRECT(E_direct);

        L_sample += E_direct * pathState.Beta / float(gSettings.DirectNumSamples);
    }

    float3 wi;
    float pdf_bxdf;
    bool isDirac;
    LobeTag lobe;
    float3 E_indirect = SampleIndirectLighting(rng.BxDF, hitInfo, wo, wi, pdf_bxdf, isDirac, lobe);
    DBG_SCALE_INTENSITY_INDIRECT(E_indirect);

    pathState.Beta *= E_indirect;
    pathState.Desc.Direction = wi;
    pathState.Desc.Origin = hitInfo.HitPosBiased;
    pathState.LastRayWasDiracDelta = isDirac;

    if (FEAT_CORE(NEE))
        pathState.LastBxdfPdf = pdf_bxdf;

    if (FEAT_CORE(GradientDomain))
    {
        vInfo.Type = VertexType::eStandard;
        vInfo.Lobe = lobe;
        vInfo.Position = hitInfo.HitPos;
        vInfo.Ng_ff = hitInfo.Ng_ff;
        vInfo.Wo = wo;
        vInfo.Wi = wi;
        vInfo.Eta = ComputeEtaAir(hitInfo.IsEntering, hitInfo.Mat.IOR_N);
        vInfo.Roughness = hitInfo.Mat.Roughness;
        vInfo.IndirectContribution = E_indirect;
        vInfo.DirectContribution = E_direct;
        vInfo.PDF *= pdf_bxdf;

        if (FEAT_CORE(GdptHybridShift))
            vInfo.RNG = rng;
    }
}

#endif