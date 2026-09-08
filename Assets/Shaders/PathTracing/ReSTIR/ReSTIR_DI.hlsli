#ifndef H_TARGET_H
#define H_TARGET_H

#include "BxDFs/GetBxDF.hlsli"
#include "PathTracing/NEE/SampleLight.hlsli"
#include "PathTracing/NEE/TraceShadowRay.hlsli"

#include "PathTracing/ReSTIR/ReSTIR_DI_Structs.h"
#include "PathTracing/ReSTIR/ReservoirBuffer.hlsli"
#include "PathTracing/ReSTIR/WRS.hlsli"

LightSampleSelection Generate(inout RngInfo rngInfo, LightSampleSelectionInfo info)
{
    BxDF bxdf;
    PathState pathState;

    float3 radiance_light;
    float pdf_light;
    float3 wi;
    uint lightIdx;
    SampleLight(rngInfo, info.HitInfo, pathState, bxdf, info.Dir_wo, info.HitPosOffset, radiance_light, lightIdx, pdf_light, wi);

    LightSampleSelection X_i;
    X_i.Direction = normalize(wi);
    X_i.LightIndex = lightIdx;
    X_i.Radiance = radiance_light;
    X_i.PDF = pdf_light;
    return X_i;
}

float Target(LightSampleSelection X_i, LightSampleSelectionInfo info)
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

float PDF(LightSampleSelection X_i, LightSampleSelectionInfo info)
{
    //return max(1e-6f, X_i.PDF);
    return X_i.PDF;
}

float3 SampleReservoir(HitInfo hitInfo, uint2 pixelCoord, float3 wo, float3 nextOrigin, BxDF bxdf)
{
    uint reservoirIdx = GetReservoirBufferIndex_Current(pixelCoord);
    ReservoirDI reservoir = gReservoirBuffer[reservoirIdx];

    DBG_OUTPUT3(Palette(reservoirIdx), RESTIR_ReservoirIdx);
    DBG_OUTPUT1(reservoir.WeightSum, RESTIR_ReservoirWeightSum);
    DBG_OUTPUT1(reservoir.Confidence / 30, RESTIR_ReservoirConfidence);
    DBG_OUTPUT3(reservoir.Y.Direction, RESTIR_YDir);
    DBG_OUTPUT3(reservoir.Y.Radiance, RESTIR_YRadiance);
    DBG_OUTPUT3(Palette(reservoir.Y.LightIndex), RESTIR_YLightIndex);
    DBG_OUTPUT1(max(0, dot(reservoir.Y.Direction, hitInfo.Ns_ff)), RESTIR_NdL);

    if (reservoir.WeightSum <= 0.0f || reservoir.Confidence <= 0.0f)
        return 0;

    float3 wi = reservoir.Y.Direction;
    float NdL = max(0, dot(wi, hitInfo.Ns_ff));
    if (NdL <= 0)
        return 0;

    bool occluded;
    float lightDistance;
    TraceShadowRay(nextOrigin, wi, occluded, lightDistance);

    DBG_OUTPUT1(occluded, RESTIR_Occluded);

    if (occluded)
        return 0;

    float3 f_bxdf;
    float pdf_bxdf;
    bxdf.Evaluate(hitInfo, wo, wi, f_bxdf, pdf_bxdf);

    float W_Y = reservoir.W_Y / reservoir.Confidence; // ?

    DBG_OUTPUT3(f_bxdf, RESTIR_Bxdf);
    DBG_OUTPUT1(pdf_bxdf, RESTIR_PDF);
    DBG_OUTPUT1(W_Y, RESTIR_W_Y);

    return f_bxdf * reservoir.Y.Radiance * NdL * W_Y;
}

#endif