#ifndef H_HIT_H
#define H_HIT_H

#include "PathTracing/Debug/Internal/OutputColorMacros.hlsli"
#include "PathTracing/Debug/Scales.hlsli"

#include "PathTracing/HitInfo/GetHitInfo.hlsli"
#include "BxDFs/GetBxDF.hlsli"

#include "Utils/RandomDirection.h"

#include "PathTracing/5_SampleDirect.hlsli"
#include "PathTracing/5_SampleIndirect.hlsli"
#include "PathTracing/EvaluateHitEmission.hlsli"

void Hit(inout PathState pathState,
        inout RngInfo rngInfo,

        HitInfo hitInfo,
        uint2 pixelCoord,

        out float3 L_sample,
        inout PathVertexInfo currentVertexInfo)
{
    L_sample = 0.0f;

    float3 wo = -pathState.Desc.Direction;

    BxDF bxdf;

    float3 E_direct = 0;
    if (FEAT_CORE(NEE))
    {
        for (uint i = 0; i < gSettings.DirectNumSamples; i++)
        {
            E_direct += SampleDirectLighting(rngInfo, hitInfo, pathState, bxdf, pixelCoord, wo, currentVertexInfo);
        }

        DBG_SCALE_INTENSITY_DIRECT(E_direct);

        L_sample += E_direct * pathState.Beta / float(gSettings.DirectNumSamples);
    }

    float3 wi;
    float pdf_bxdf;
    bool isDirac;
    bool isGlossy;
    float3 E_indirect = SampleIndirectLighting(rngInfo, hitInfo, bxdf, wo, wi, pdf_bxdf, isDirac, isGlossy);
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

        //currentVertexInfo.Type = isGlossy ? VertexType::eGlossy : VertexType::eDiffuse; // TODO
        currentVertexInfo.Type = GetIsVertexDiffuse(hitInfo.Mat.Roughness) ? VertexType::eDiffuse : VertexType::eGlossy;
        currentVertexInfo.Position = hitInfo.HitPos;
        currentVertexInfo.SFrame = hitInfo.SFrame;
        currentVertexInfo.Wo = wo;
        currentVertexInfo.Wi = wi;
        currentVertexInfo.Eta = eta;
        currentVertexInfo.IndirectContribution = E_indirect;
        currentVertexInfo.DirectContribution = E_direct;
        currentVertexInfo.PDF *= pdf_bxdf;
    }
}

#endif