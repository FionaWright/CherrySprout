#ifndef H_HIT_H
#define H_HIT_H

#include "PathTracing/Debug/Internal/OutputColorMacros.hlsli"
#include "PathTracing/Debug/Scales.hlsli"

#include "PathTracing/HitInfo/GetHitInfo.hlsli"
#include "BxDFs/GetBxDF.hlsli"

#include "Utils/RandomDirection.h"

#include "PathTracing/5_SampleDirect.hlsli"
#include "PathTracing/5_SampleIndirect.hlsli"
#include "PathTracing/NEE/SampleEmissive.hlsli"
#include "PathTracing/MIS.hlsli"

void Hit(inout PathState pathState,
        inout RngInfo rngInfo,

        HitInfo hitInfo,
        uint2 pixelCoord,

        out float3 L_sample,
        inout PathVertexInfo currentVertexInfo)
{
    float3 nextOrigin = hitInfo.HitPos + hitInfo.Ng_ff * EPSILON;
    float3 wo = -pathState.Desc.Direction;

    float3 L_emission = hitInfo.Emission;
    if (FEATURE_ENABLED(NEE) && any(L_emission > 0) && !pathState.IsPrimaryRay && !pathState.LastRayWasDiracDelta)
    {
        float pdf_emissive = EvaluateEmissivePdf(hitInfo.PrimitiveCount, hitInfo.TriangleArea, hitInfo.Ng_ff, wo, hitInfo.RayT);
        float m = PowerHeuristic(pathState.LastBxdfPdf, pdf_emissive, 1, gSettings.DirectNumSamples);
        L_emission *= m;
    }
    DBG_SCALE_INTENSITY_EMISSION(L_emission);
    L_sample = pathState.Beta * L_emission;

    BxDF bxdf;

    float3 E_direct = 0;
    if (FEATURE_ENABLED(NEE))
    {
        for (uint i = 0; i < gSettings.DirectNumSamples; i++)
        {
            E_direct += SampleDirectLighting(rngInfo, hitInfo, pathState, bxdf, pixelCoord, wo, nextOrigin, currentVertexInfo);
        }

        DBG_SCALE_INTENSITY_DIRECT(E_direct);

        L_sample += E_direct * pathState.Beta / float(gSettings.DirectNumSamples);
    }

    float3 wi;
    float pdf_bxdf;
    float3 E_indirect = SampleIndirectLighting(rngInfo, hitInfo, bxdf, pathState, wo, wi, pdf_bxdf);
    DBG_SCALE_INTENSITY_INDIRECT(E_indirect);

    pathState.Beta *= E_indirect;

    if (FEATURE_ENABLED(NEE))
        pathState.LastBxdfPdf = pdf_bxdf;

    pathState.Desc.Direction = wi;
    pathState.Desc.Origin = nextOrigin;

    if (FEATURE_ENABLED(GradientDomain))
    {
        float iorNCurrent =  hitInfo.IsEntering ? IOR_N_AIR          : hitInfo.Mat.IOR_N;
        float iorNNext =     hitInfo.IsEntering ? hitInfo.Mat.IOR_N  : IOR_N_AIR;
        float eta = iorNCurrent / iorNNext;

        currentVertexInfo.Type = GetIsVertexDiffuse(hitInfo.Mat.Roughness) ? VertexType::eDiffuse : VertexType::eGlossy;
        currentVertexInfo.Position = nextOrigin; // TODO: Use hitpos?
        currentVertexInfo.SFrame = hitInfo.SFrame;
        currentVertexInfo.Wo = wo;
        currentVertexInfo.Wi = wi;
        currentVertexInfo.Eta = eta;
        currentVertexInfo.IndirectContribution = E_indirect;
        currentVertexInfo.DirectContribution = E_direct;
        currentVertexInfo.PDF_Bxdf = pdf_bxdf;
        currentVertexInfo.PDF *= pdf_bxdf;
    }
}

#endif