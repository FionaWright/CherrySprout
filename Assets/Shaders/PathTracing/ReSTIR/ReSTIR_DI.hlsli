#ifndef H_TARGET_H
#define H_TARGET_H

#if FEAT_CORE_D(RestirDI)

#include "BxDFs/GetBxDF.hlsli"
#include "PathTracing/NEE/SampleLight.hlsli"
#include "PathTracing/VisibilityRays.hlsli"

#include "PathTracing/ReSTIR/ReSTIR_DI_Structs.h"
#include "PathTracing/ReSTIR/ReservoirBuffer.hlsli"
#include "PathTracing/ReSTIR/WRS.hlsli"
#include "Utils/Debug/Palette.h"
#include "PathTracing/Debug/Assert.hlsli"

SubReservoir_DI Generate(inout RngInfo rngInfo, SubReservoirInfo_DI info)
{
    BxDF bxdf;

    LightEval lightEval;
    LightSample lightSample = SampleLight(rngInfo, info.HitInfo.HitPos, bxdf, info.Wo, lightEval);

    SubReservoir_DI X_i;
    X_i.LightSample = lightSample;
    X_i.LightEval = lightEval;
    return X_i;
}

void ReEvaluate(inout SubReservoir_DI X_i, SubReservoirInfo_DI info)
{
    BxDF bxdf;
    X_i.LightEval = EvaluateLight(info.HitInfo.HitPos, bxdf, info.Wo, X_i.LightSample);
}

float Target(SubReservoir_DI X_i, SubReservoirInfo_DI info)
{
    float NdL = max(0.0f, dot(info.HitInfo.Ns_ff, X_i.LightEval.Direction));
    if (NdL <= 0)
        return 0.0f;

    if (X_i.LightEval.PDF < EPSILON || length(X_i.LightEval.Radiance) <= 0.0f)
        return 0.0f;

    float shadowFactor = 1.0f;
    if (FEAT_CORE(RestirTargetVisibility))
    {
        TraceRayShadow(info.HitInfo.HitPosBiased, X_i.LightEval.Direction, X_i.LightEval.Distance, shadowFactor);
        if (shadowFactor <= 0.0f)
            return 0;
    }

    float3 m;
    if (X_i.LightEval.IsDelta)
    {
        m = 1.0f;
    }
    else
    {
        BxDF bxdf;
        BxdfEval bxdfEval = bxdf.Evaluate(info.HitInfo, info.Wo, X_i.LightEval.Direction);

        m = bxdfEval.Radiance * PowerHeuristic(X_i.LightEval.PDF, bxdfEval.PDF, 1, 1);
    }

    float3 radiance = shadowFactor * X_i.LightEval.Radiance * m * NdL / max(1e-6f, X_i.LightEval.PDF);
    return Luminance(radiance);
}

float PDF(SubReservoir_DI X_i, SubReservoirInfo_DI info)
{
    return X_i.LightEval.PDF;
}

bool TrySampleReservoir(
    HitInfo hitInfo,
    uint2 pixelCoord,
    float3 wo,
    BxDF bxdf,
    uint numSamples,
    out SubReservoir_DI Y,
    out float lightDistance,
    out float3 radiance)
{
    radiance = 0.0f;

    uint reservoirIdx = GetReservoirBufferIndex_Final(pixelCoord);
    ReservoirDI reservoir = gReservoirBuffer[reservoirIdx];

    Y = reservoir.Y;
    float3 wi = Y.LightEval.Direction;
    lightDistance = Y.LightEval.Distance;

    // Debug
    {
        DBG_OUTPUT3(Palette(reservoirIdx), RESTIR_ReservoirIdx);
        DBG_OUTPUT1(reservoir.WeightSum, RESTIR_ReservoirWeightSum);
        DBG_OUTPUT1(reservoir.Confidence / 30, RESTIR_ReservoirConfidence);
        DBG_OUTPUT3(wi, RESTIR_YDir);
        DBG_OUTPUT3(Y.LightEval.Radiance, RESTIR_YRadiance);
        DBG_OUTPUT1(Y.LightSample.Index, RESTIR_YLightIndex);
        DBG_OUTPUT1(max(0, dot(wi, hitInfo.Ns_ff)), RESTIR_NdL);
        DBG_OUTPUT1(reservoir.W_Y, RESTIR_W_Y);
        DBG_ASSERT_GE(reservoir.W_Y, 0.0f, RESTIR_W_Y_RANGE);
        DBG_ASSERT_GE(reservoir.WeightSum, 0.0f, RESTIR_WEIGHT_SUM_RANGE);
        DBG_ASSERT_RANGE(0.0f, reservoir.Confidence, gSettings.RestirGenConfidenceCap, RESTIR_CONF_RANGE);
    }

    if (reservoir.Confidence <= 0.0f || reservoir.W_Y <= 0.0f)
        return false;

    //LightEval lightEval = EvaluateLight(hitInfo.HitPos, bxdf, wo, Y.LightSample);
    LightEval lightEval = Y.LightEval;

    float NdL = max(0, dot(wi, hitInfo.Ns_ff));
    if (NdL <= 0 || length(lightEval.Radiance) <= 0.0f)
        return false;

    float shadowFactor;
    TraceRayShadow(hitInfo.HitPosBiased, wi, lightDistance, shadowFactor);
    DBG_OUTPUT1(shadowFactor, RESTIR_Occluded);
    if (shadowFactor <= 0.0f)
        return false;

    //DBG_ASSERT_APPROX(length(wi), 1.0f, 0.02f, RESTIR_DIR_NORM); // TODO: Seems to cause issues for some reason

    float3 m;
    if (lightEval.IsDelta)
    {
        m = 1.0f;
    }
    else
    {
        BxdfEval bxdfEval = bxdf.Evaluate(hitInfo, wo, wi);

        m = bxdfEval.Radiance * PowerHeuristic(lightEval.PDF, bxdfEval.PDF, numSamples, 1);

        DBG_OUTPUT3(bxdfEval.Radiance, RESTIR_Bxdf);
        DBG_OUTPUT1(bxdfEval.PDF, RESTIR_PDF);
    }

    radiance = shadowFactor * lightEval.Radiance * m * NdL * reservoir.W_Y;
    return true;
}

#else

SubReservoir_DI Generate(inout RngInfo rngInfo, SubReservoirInfo_DI info) { return (SubReservoir_DI)0; }
float Target(SubReservoir_DI X_i, SubReservoirInfo_DI info) { return NAN; }
float PDF(SubReservoir_DI X_i, SubReservoirInfo_DI info) { return NAN; }
bool TrySampleReservoir(
    HitInfo hitInfo,
    uint2 pixelCoord,
    float3 wo,
    BxDF bxdf,
    uint numSamples,
    out SubReservoir_DI Y,
    out float lightDistance,
    out float3 radiance) { Y = (SubReservoir_DI)0; lightDistance = NAN; radiance = NAN; return NAN; }

#endif

#endif