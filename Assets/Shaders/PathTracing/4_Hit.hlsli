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
        inout PathVertexInfo currentVertexInfo)
{
    L_sample = 0.0f;

    float3 wo = -pathState.Desc.Direction;

    float3 E_direct = 0;
    if (FEAT_CORE(NEE))
    {
        for (uint i = 0; i < gSettings.DirectNumSamples; i++)
        {
            E_direct += SampleDirectLighting(rng.NEE[i], hitInfo, pathState, pixelCoord, wo, currentVertexInfo);
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
        float iorNCurrent =  hitInfo.IsEntering ? IOR_N_AIR          : hitInfo.Mat.IOR_N;
        float iorNNext =     hitInfo.IsEntering ? hitInfo.Mat.IOR_N  : IOR_N_AIR;
        float eta = iorNCurrent / iorNNext;

        currentVertexInfo.Type = VertexType::eStandard;
        currentVertexInfo.Lobe = lobe;
        currentVertexInfo.Position = hitInfo.HitPos;
        currentVertexInfo.Ng_ff = hitInfo.Ng_ff;
        currentVertexInfo.Wo = wo;
        currentVertexInfo.Wi = wi;
        currentVertexInfo.Eta = eta;
        currentVertexInfo.Roughness = hitInfo.Mat.Roughness;
        currentVertexInfo.IndirectContribution = E_indirect;
        currentVertexInfo.DirectContribution = E_direct;
        currentVertexInfo.PDF *= pdf_bxdf;

        if (FEAT_CORE(GdptHybridShift))
            currentVertexInfo.RNG = rng;
    }
}

#endif