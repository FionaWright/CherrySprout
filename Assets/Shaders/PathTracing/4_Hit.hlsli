#ifndef H_HIT_H
#define H_HIT_H

#include "PathTracing/Debug/OutputColorMacros.hlsli"

#include "PathTracing/HitInfo/GetHitInfo.hlsli"
#include "BxDFs/GetBxDF.hlsli"

#include "Utils/RandomDirection.h"

#include "PathTracing/5_SampleDirect.hlsli"
#include "PathTracing/5_SampleIndirect.hlsli"

void Hit(inout PathState pathState,
        inout RngInfo rngInfo,

        HitInfo hitInfo,
        uint2 pixelCoord,

        out float3 L_sample,
        out PathVertexInfo currentVertexInfo)
{
    float3 hitPos = pathState.Desc.Origin + pathState.Desc.Direction * hitInfo.RayT;
    float3 nextOrigin = hitPos + hitInfo.Ng_ff * EPSILON;

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
    float pdf_bxdf;
    float3 E_indirect = SampleIndirectLighting(rngInfo, hitInfo, bxdf, pathState, wo, wi, pdf_bxdf);
    pathState.Beta *= E_indirect;

    if (FEATURE_ENABLED(NEE))
        pathState.LastBxdfPdf = pdf_bxdf;

    if (FEATURE_ENABLED(GradientDomain))
        pathState.PDF *= pdf_bxdf;

    pathState.Desc.Direction = wi;
    pathState.Desc.Origin = nextOrigin;

    if (FEATURE_ENABLED(GradientDomain))
    {
        float iorNCurrent =  hitInfo.IsEntering ? IOR_N_AIR          : hitInfo.Mat.IOR_N;
        float iorNNext =     hitInfo.IsEntering ? hitInfo.Mat.IOR_N  : IOR_N_AIR;
        float eta = iorNCurrent / iorNNext;

        currentVertexInfo.Type = GetIsVertexDiffuse(hitInfo.Mat.Roughness) ? VertexType::eDiffuse : VertexType::eGlossy;
        currentVertexInfo.Position = hitPos;
        currentVertexInfo.SFrame = hitInfo.SFrame;
        currentVertexInfo.Wo = wo;
        currentVertexInfo.Wi = wi;
        currentVertexInfo.Eta = eta;
        currentVertexInfo.IndirectContribution = E_indirect;
        currentVertexInfo.BxdfPdf = pdf_bxdf;
    }
}

#endif