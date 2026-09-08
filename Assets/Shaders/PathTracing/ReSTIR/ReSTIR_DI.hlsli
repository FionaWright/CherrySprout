#ifndef H_TARGET_H
#define H_TARGET_H

#include "BxDFs/GetBxDF.hlsli"
#include "PathTracing/NEE/NEE.hlsli"

#include "PathTracing/ReSTIR/ReSTIR_DI_Structs.h"
#include "PathTracing/ReSTIR/ReservoirBuffer.hlsli"
#include "PathTracing/ReSTIR/WRS.hlsli"
#include "Utils/Debug/Palette.h"

LightSample Generate(inout RngInfo rngInfo, LightSampleSelectionInfo info)
{
    BxDF bxdf;
    PathState pathState;

    return SampleLight(rngInfo, info.HitInfo, pathState, bxdf, info.Dir_wo, info.HitPos, info.HitPosOffset);
}

float Target(LightSample X_i, LightSampleSelectionInfo info)
{
    //float NdL = abs(dot(info.HitInfo.Ns_ff, X_i.Direction));
    float NdL = max(0.0f, dot(info.HitInfo.Ns_ff, X_i.Direction));

    if (X_i.PDF < EPSILON || NdL <= 0)
        return 0.0f; // Null?

    BxDF bxdf;
    float3 f_bxdf;
    float pdf_bxdf;
    bxdf.Evaluate(info.HitInfo, info.Dir_wo, X_i.Direction, f_bxdf, pdf_bxdf);

    float3 radiance = f_bxdf * X_i.Radiance * NdL;
    return Luminance(radiance);
}

float PDF(LightSample X_i, LightSampleSelectionInfo info)
{
    //return max(1e-6f, X_i.PDF);
    return X_i.PDF;
}

float3 SampleReservoir(HitInfo hitInfo, uint2 pixelCoord, float3 wo, float3 nextOrigin, BxDF bxdf, out LightSample lightSample)
{
    uint reservoirIdx = GetReservoirBufferIndex_Current(pixelCoord);
    ReservoirDI reservoir = gReservoirBuffer[reservoirIdx];

    lightSample = reservoir.Y;

    DBG_OUTPUT3(Palette(reservoirIdx), RESTIR_ReservoirIdx);
    DBG_OUTPUT1(reservoir.WeightSum, RESTIR_ReservoirWeightSum);
    DBG_OUTPUT1(reservoir.Confidence / 30, RESTIR_ReservoirConfidence);
    DBG_OUTPUT3(reservoir.Y.Direction, RESTIR_YDir);
    DBG_OUTPUT3(reservoir.Y.Radiance, RESTIR_YRadiance);
    DBG_OUTPUT3(Palette(reservoir.Y.Index), RESTIR_YLightIndex);
    DBG_OUTPUT1(max(0, dot(reservoir.Y.Direction, hitInfo.Ns_ff)), RESTIR_NdL);

    float NdL = max(0, dot(reservoir.Y.Direction, hitInfo.Ns_ff));
    if (reservoir.WeightSum <= 0.0f || reservoir.Confidence <= 0.0f || NdL <= 0)
    {
        DBG_OUTPUT1(1, RESTIR_Occluded);
        return 0;
    }

    bool occluded;
    TraceShadowRay(nextOrigin, reservoir.Y.Direction, occluded, reservoir.Y.Distance);

    DBG_OUTPUT1(occluded, RESTIR_Occluded);

    if (occluded)
        return 0;

    float3 m;
    if (reservoir.Y.IsDelta)
    {
        m = 1.0f;
    }
    else
    {
        float3 f_bxdf;
        float pdf_bxdf;
        bxdf.Evaluate(hitInfo, wo, reservoir.Y.Direction, f_bxdf, pdf_bxdf);

        m = f_bxdf * PowerHeuristic(reservoir.Y.PDF, pdf_bxdf);

        DBG_OUTPUT3(f_bxdf, RESTIR_Bxdf);
        DBG_OUTPUT1(pdf_bxdf, RESTIR_PDF);
    }

    float W_Y = reservoir.W_Y / reservoir.Confidence; // ?
    DBG_OUTPUT1(W_Y, RESTIR_W_Y);

    return reservoir.Y.Radiance * m * max(0, NdL) * W_Y;
}

#endif