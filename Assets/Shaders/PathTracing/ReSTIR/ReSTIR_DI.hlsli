#ifndef H_TARGET_H
#define H_TARGET_H

#if FEATURE_ENABLED(RestirDI)

#include "BxDFs/GetBxDF.hlsli"
#include "PathTracing/NEE/NEE.hlsli"

#include "PathTracing/ReSTIR/ReSTIR_DI_Structs.h"
#include "PathTracing/ReSTIR/ReservoirBuffer.hlsli"
#include "PathTracing/ReSTIR/WRS.hlsli"
#include "Utils/Debug/Palette.h"
#include "PathTracing/Debug/Assert.hlsli"

LightSample Generate(inout RngInfo rngInfo, LightSampleSelectionInfo info)
{
    BxDF bxdf;
    PathState pathState;

    return SampleLight(rngInfo, info.HitInfo, pathState, bxdf, info.Dir_wo, info.HitPos, info.HitPosOffset);
}

float Target(LightSample X_i, LightSampleSelectionInfo info)
{
    float NdL = max(0.0f, dot(info.HitInfo.Ns_ff, X_i.Direction));

    if (X_i.PDF < EPSILON || NdL <= 0)
        return 0.0f;

    if (FEATURE_ENABLED(RestirTargetVisibility))
    {
        bool occluded;
        TraceRayShadow(info.HitPosOffset, X_i.Direction, occluded, X_i.Distance);
        if (occluded)
            return 0;
    }

    float3 m;
    if (X_i.IsDelta)
    {
        m = 1.0f;
    }
    else
    {
        BxDF bxdf;
        float3 f_bxdf;
        float pdf_bxdf;
        bxdf.Evaluate(info.HitInfo, info.Dir_wo, X_i.Direction, f_bxdf, pdf_bxdf);

        m = f_bxdf * PowerHeuristic(X_i.PDF, pdf_bxdf, 1, 1); // TODO: Send in M?
    }

    float3 radiance = X_i.Radiance * m * NdL;
    return Luminance(radiance);
}

float PDF(LightSample X_i, LightSampleSelectionInfo info)
{
    return X_i.PDF;
}

float3 SampleReservoir(HitInfo hitInfo, uint2 pixelCoord, float3 wo, float3 nextOrigin, BxDF bxdf, uint numSamples, out LightSample lightSample)
{
    uint reservoirIdx = GetReservoirBufferIndex_Current(pixelCoord);
    ReservoirDI reservoir = gReservoirBuffer[reservoirIdx];

    lightSample = reservoir.Y;

    // Debug
    {
        DBG_OUTPUT3(Palette(reservoirIdx), RESTIR_ReservoirIdx);
        DBG_OUTPUT1(reservoir.WeightSum, RESTIR_ReservoirWeightSum);
        DBG_OUTPUT1(reservoir.Confidence / 30, RESTIR_ReservoirConfidence);
        DBG_OUTPUT3(reservoir.Y.Direction, RESTIR_YDir);
        DBG_OUTPUT3(reservoir.Y.Radiance, RESTIR_YRadiance);
        DBG_OUTPUT3(Palette(reservoir.Y.Index), RESTIR_YLightIndex);
        DBG_OUTPUT1(max(0, dot(reservoir.Y.Direction, hitInfo.Ns_ff)), RESTIR_NdL);
        DBG_OUTPUT1(reservoir.W_Y, RESTIR_W_Y);
        DBG_ASSERT_GE(reservoir.W_Y, 0.0f, RESTIR_W_Y_RANGE);
        DBG_ASSERT_GE(reservoir.WeightSum, 0.0f, RESTIR_WEIGHT_SUM_RANGE);
        DBG_ASSERT_RANGE(0.0f, RESTIR_CONF_RANGE, gSettings.RestirConfidenceCap, RESTIR_CONF_RANGE);
    }

    float NdL = max(0, dot(reservoir.Y.Direction, hitInfo.Ns_ff));
    if (reservoir.W_Y <= 0.0f || reservoir.Confidence <= 0.0f || NdL <= 0)
        return 0;

    bool occluded;
    TraceRayShadow(nextOrigin, reservoir.Y.Direction, occluded, reservoir.Y.Distance);

    DBG_OUTPUT1(occluded, RESTIR_Occluded);
    //DBG_ASSERT_APPROX(length(reservoir.Y.Direction), 1.0f, 0.02f, RESTIR_DIR_NORM); // TODO: Seems to cause issues for some reason

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

        m = f_bxdf * PowerHeuristic(reservoir.Y.PDF, pdf_bxdf, numSamples, 1);

        DBG_OUTPUT3(f_bxdf, RESTIR_Bxdf);
        DBG_OUTPUT1(pdf_bxdf, RESTIR_PDF);
    }

    return reservoir.Y.Radiance * m * NdL * reservoir.W_Y;
    //return reservoir.Y.Radiance * m * NdL  / reservoir.Y.PDF;
}

#else

LightSample Generate(inout RngInfo rngInfo, LightSampleSelectionInfo info) { return (LightSample)0; }
float Target(LightSample X_i, LightSampleSelectionInfo info) { return NAN; }
float PDF(LightSample X_i, LightSampleSelectionInfo info) { return NAN; }
float3 SampleReservoir(HitInfo hitInfo, uint2 pixelCoord, float3 wo, float3 nextOrigin, BxDF bxdf, uint numSamples, out LightSample lightSample) { return NAN; }

#endif

#endif