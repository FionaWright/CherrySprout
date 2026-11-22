#ifndef H_TARGET_H
#define H_TARGET_H

#include "BxDFs/GetBxDF.hlsli"

#if FEAT_ADV_D(RestirDI)

#include "PathTracing/RandomSample.hlsli"
#include "PathTracing/NEE/SampleLight.hlsli"
#include "PathTracing/VisibilityRays.hlsli"

#include "PathTracing/ReSTIR/Structs.h"
#include "PathTracing/ReSTIR/ReservoirBuffer.hlsli"
#include "PathTracing/ReSTIR/WRS.hlsli"
#include "Utils/Debug/Palette.h"
#include "PathTracing/Debug/Assert.hlsli"
#include "PathTracing/Debug/SettingGuards.hlsli"

template <typename TPrimaryRayInfo>
void Generate(inout RngInfo rngInfo, TPrimaryRayInfo primaryRayInfo, out CandidateDI X_i)
{
    RngValuesAncillary::RngValuesNEE rng = SampleRngNEE(rngInfo);

    LightEval lightEval;
    LightSample lightSample = SampleLight(rng, primaryRayInfo.HitInfo.HitPos, lightEval);

    X_i.LightSample = lightSample;
    X_i.LightEval = lightEval;
}

template <typename TPrimaryRayInfo>
void Shift(inout CandidateDI X_i, TPrimaryRayInfo primaryRayInfo)
{
    X_i.LightEval = EvaluateLight(primaryRayInfo.HitInfo.HitPos, X_i.LightSample);
}

template <typename TPrimaryRayInfo>
float Target(CandidateDI X_i, TPrimaryRayInfo primaryRayInfo)
{
    float NdL = max(0.0f, dot(primaryRayInfo.HitInfo.Ns_ff, X_i.LightEval.Direction));
    if (NdL <= 0)
        return 0.0f;

    if (X_i.LightEval.PDF < EPSILON || length(X_i.LightEval.Radiance) <= 0.0f)
        return 0.0f;

    float shadowFactor = 1.0f;
    if (FEAT_ADV(RestirTargetVisibility))
    {
        TraceRayShadow(primaryRayInfo.HitInfo.HitPosBiased, X_i.LightEval.Direction, X_i.LightEval.Distance, shadowFactor);
        if (shadowFactor <= 0.0f)
            return 0;
    }

    float3 wo = -primaryRayInfo.CenterRayDirection;
    BxdfEval bxdfEval = BxDF::Evaluate(primaryRayInfo.HitInfo, wo, X_i.LightEval.Direction, LobeTag::eAll);

    float3 radiance = bxdfEval.Radiance * X_i.LightEval.Radiance;

    float m;
    if (FEAT_DBG(FixedMisNeeManual))
    {
        m = GetFixedMisNee();
    }
    else if (X_i.LightEval.IsDelta)
    {
        m = 1.0f;
    }
    else
    {
        m = BalanceHeuristic(X_i.LightEval.PDF, bxdfEval.PDF, 1, 1);
    }

    float3 E_direct = shadowFactor * radiance * m * NdL; // Note: Don't divide by PDF here
    return Luminance(E_direct);
}

template <typename TPrimaryRayInfo>
float PDF(CandidateDI X_i, TPrimaryRayInfo primaryRayInfo)
{
    return X_i.LightEval.PDF;
}

template <typename TReservoir>
bool TrySampleReservoirDI(
    HitInfo hitInfo,
    uint2 pixelCoord,
    float3 wo,
    uint numSamples,
    out CandidateDI Y,
    out float lightDistance,
    out float3 E_direct)
{
    E_direct = 0.0f;

    uint reservoirIdx = GetReservoirBufferIndex_Final(pixelCoord);
    TReservoir reservoir = gReservoirBufferDI[reservoirIdx];

    Y = reservoir.Y;
    float3 wi = Y.LightEval.Direction;
    lightDistance = Y.LightEval.Distance;

    // Debug
    {
        DBG_OUTPUT3(Palette(reservoirIdx),          RESTIR_ReservoirIdx);
        DBG_OUTPUT1(reservoir.WeightSum,            RESTIR_ReservoirWeightSum);
        DBG_OUTPUT1(reservoir.Confidence / 30,      RESTIR_ReservoirConfidence);
        DBG_OUTPUT1(max(0, dot(wi, hitInfo.Ns_ff)), RESTIR_NdL);
        DBG_OUTPUT1(reservoir.W_Y,                  RESTIR_W_Y);

        DBG_OUTPUT3(wi,                             RESTIRDI_Y_Dir);
        DBG_OUTPUT3(Y.LightEval.Radiance,           RESTIRDI_Y_Radiance);
        DBG_OUTPUT1(Y.LightSample.Index,            RESTIRDI_Y_LightIndex);

        DBG_ASSERT_GE(reservoir.W_Y, 0.0f,          RESTIR_W_Y_RANGE);
        DBG_ASSERT_GE(reservoir.WeightSum, 0.0f,    RESTIR_WEIGHT_SUM_RANGE);
    }

    if (reservoir.Confidence <= 0.0f || reservoir.W_Y <= 0.0f)
        return false;

    LightEval lightEval = Y.LightEval;

    float NdL = max(0, dot(wi, hitInfo.Ns_ff));
    if (NdL <= 0 || length(lightEval.Radiance) <= 0.0f)
        return false;

    float shadowFactor;
    TraceRayShadow(hitInfo.HitPosBiased, wi, Y.LightEval.Distance, shadowFactor);
    DBG_OUTPUT1(shadowFactor, RESTIRDI_ShadowFactor);
    if (shadowFactor <= 0.0f)
        return false;

    BxdfEval bxdfEval = BxDF::Evaluate(hitInfo, wo, wi, LobeTag::eAll);
    float3 radiance = bxdfEval.Radiance * lightEval.Radiance;

    DBG_OUTPUT3(bxdfEval.Radiance, RESTIRDI_Bxdf);
    DBG_OUTPUT1(bxdfEval.PDF,      RESTIRDI_PDF);

    float m;
    if (lightEval.IsDelta)
    {
        m = 1.0f;
    }
    else
    {
        m = BalanceHeuristic(lightEval.PDF, bxdfEval.PDF, numSamples, 1);
    }

    E_direct = shadowFactor * radiance * m * NdL * reservoir.W_Y;
    return true;
}

#else

template <typename TPrimaryRayInfo> void Generate(inout RngInfo rngInfo, TPrimaryRayInfo primaryRayInfo, out CandidateDI X_i) { X_i = (CandidateDI)0; }
template <typename TPrimaryRayInfo> void Shift(inout CandidateDI X_i, TPrimaryRayInfo primaryRayInfo) {}
template <typename TPrimaryRayInfo> float Target(CandidateDI X_i, TPrimaryRayInfo primaryRayInfo) { return NAN; }
template <typename TPrimaryRayInfo> float PDF(CandidateDI X_i, TPrimaryRayInfo primaryRayInfo) { return NAN; }
template <typename TReservoir> bool TrySampleReservoirDI(
    HitInfo hitInfo,
    uint2 pixelCoord,
    float3 wo,
    uint numSamples,
    out CandidateDI Y,
    out float lightDistance,
    out float3 radiance) { Y = (CandidateDI)0; lightDistance = NAN; radiance = NAN; return NAN; }

#endif

#endif