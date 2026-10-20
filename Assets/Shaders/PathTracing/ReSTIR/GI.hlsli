#ifndef H_RESTIR_GI_H
#define H_RESTIR_GI_H

#if FEAT_CORE_D(RestirGI)

#include "BxDFs/GetBxDF.hlsli"
#include "PathTracing/NEE/SampleLight.hlsli"
#include "PathTracing/VisibilityRays.hlsli"

#include "PathTracing/ReSTIR/Structs.h"
#include "PathTracing/ReSTIR/ReservoirBuffer.hlsli"
#include "PathTracing/ReSTIR/WRS.hlsli"
#include "Utils/Debug/Palette.h"
#include "PathTracing/Debug/Assert.hlsli"

SubReservoir_GI Generate(inout RngInfo rngInfo, SubReservoirInfo_GI info)
{
    BxDF bxdf;
    BxdfEval bxdfEval;
    BxdfSample bxdfSample = bxdf.Sample(rngInfo, info.HitInfo, info.Wo, bxdfEval);

    bool isMiss;
    float3 hitPos;
    float3 Ns_ff;
    TraceHitData(info.HitInfo.HitPosBiased, bxdfSample.Wi, isMiss, hitPos, Ns_ff);

    SubReservoir_GI X_i;
    X_i.Position = isMiss ? INF : hitPos;
    X_i.Normal = isMiss ? 0 : Ns_ff;
    X_i.BxdfEval = bxdfEval;
    return X_i;
}

void ReEvaluate(inout SubReservoir_GI X_i, SubReservoirInfo_GI info)
{
    BxDF bxdf;
    X_i.LightEval = EvaluateLight(info.HitInfo.HitPos, bxdf, info.Wo, X_i.LightSample);
}

float Target(SubReservoir_GI X_i, SubReservoirInfo_GI info)
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
    if (FEAT_DBG(FixedMisNeeManual))
    {
        m = gDebugSettings.FixedMisNee;
    }
    else if (X_i.LightEval.IsDelta)
    {
        m = 1.0f;
    }
    else
    {
        BxDF bxdf;
        BxdfEval bxdfEval = bxdf.Evaluate(info.HitInfo, info.Wo, X_i.LightEval.Direction);

        m = bxdfEval.Radiance * PowerHeuristic(X_i.LightEval.PDF, bxdfEval.PDF, 1, 1);
    }

    float3 radiance = shadowFactor * X_i.LightEval.Radiance * m * NdL; // Note: Don't divide by PDF here
    return Luminance(radiance);
}

float PDF(SubReservoir_GI X_i, SubReservoirInfo_GI info)
{
    return X_i.LightEval.PDF;
}

bool TrySampleReservoir(
    HitInfo hitInfo,
    uint2 pixelCoord,
    float3 wo,
    BxDF bxdf,
    uint numSamples,
    out SubReservoir_GI Y,
    out float lightDistance,
    out float3 radiance)
{
    radiance = 0.0f;

    uint reservoirIdx = GetReservoirBufferIndex_Final(pixelCoord);
    ReservoirDI reservoir = gReservoirBuffer[reservoirIdx];

    Y = reservoir.Y;
    float3 wi = Y.LightEval.Direction;
    lightDistance = Y.LightEval.Distance;

    if (reservoir.Confidence <= 0.0f || reservoir.W_Y <= 0.0f)
        return false;

    LightEval lightEval = Y.LightEval;

    float NdL = max(0, dot(wi, hitInfo.Ns_ff));
    if (NdL <= 0 || length(lightEval.Radiance) <= 0.0f)
        return false;

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

SubReservoir_GI Generate(inout RngInfo rngInfo, SubReservoirInfo_GI info) { return (SubReservoir_GI)0; }
float Target(SubReservoir_GI X_i, SubReservoirInfo_GI info) { return NAN; }
float PDF(SubReservoir_GI X_i, SubReservoirInfo_GI info) { return NAN; }
bool TrySampleReservoir(
    HitInfo hitInfo,
    uint2 pixelCoord,
    float3 wo,
    BxDF bxdf,
    uint numSamples,
    out SubReservoir_GI Y,
    out float lightDistance,
    out float3 radiance) { Y = (SubReservoir_GI)0; lightDistance = NAN; radiance = NAN; return NAN; }

#endif

#endif