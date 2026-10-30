#ifndef H_RESTIR_GI_H
#define H_RESTIR_GI_H

#include "BxDFs/GetBxDF.hlsli"

#if FEAT_CORE_D(RestirGI)

#include "PathTracing/RandomSample.hlsli"
#include "PathTracing/NEE/SampleNEE.hlsli"
#include "PathTracing/VisibilityRays.hlsli"
#include "PathTracing/HitInfo/GetHitInfo.hlsli"
#include "PathTracing/4_Miss.hlsli"
#include "PathTracing/ShiftMapping/Components/Reconnect.hlsli"
#include "PathTracing/ShiftMapping/Components/ReconnectEnv.hlsli"

#include "PathTracing/ReSTIR/Structs.h"
#include "PathTracing/ReSTIR/ReservoirBuffer.hlsli"
#include "PathTracing/ReSTIR/WRS.hlsli"
#include "Utils/Debug/Palette.h"
#include "PathTracing/Debug/Assert.hlsli"

void Generate(inout RngInfo rngInfo, ContextPrimaryRay context, out CandidateGI X_i)
{
    RngValuesAncillary::RngValuesBxDF rngBxDF = SampleRngBxDF(rngInfo);

    BxdfEval bxdfEval;
    BxdfSample bxdfSample = BxDF::Sample(rngBxDF, context.HitInfo, context.Wo, bxdfEval);

    // TODO: Simpler function that doesn't extract everything?
    bool isMiss;
    HitInfo secondaryHitInfo;
    {
        RayQuery<RAY_FLAGS> q;
        RayDesc ray;
        ray.Origin = context.HitInfo.HitPosBiased;
        ray.Direction = bxdfSample.Wi;
        ray.TMin = TMIN_BIAS_COEF * EPSILON;
        ray.TMax = INF;

        q.TraceRayInline(gTLAS, RAY_FLAGS, 0xFF, ray);
        while (q.Proceed()) {}
        isMiss = q.CommittedStatus() != COMMITTED_TRIANGLE_HIT;

        if (!isMiss)
            GetHitInfo(q, secondaryHitInfo);
    }

    float3 E_direct = 0.0f;
    if (isMiss)
    {
        E_direct = Miss(bxdfSample.Wi, bxdfEval.PDF, false, bxdfSample.IsDirac);
    }
    else
    {
        RngValuesAncillary::RngValuesNEE rngNEE = SampleRngNEE(rngInfo);

        float3 wo = -bxdfSample.Wi;
        E_direct = SampleNEE(rngNEE, secondaryHitInfo, 1, wo);
    }

    // TODO: Extension: More bounces

    X_i.SourcePositionBiased = context.HitInfo.HitPosBiased;
    X_i.DestPosition = isMiss ? bxdfSample.Wi : secondaryHitInfo.HitPos;
    X_i.DestNg = isMiss ? 0 : secondaryHitInfo.Ng;
    X_i.IsMiss = isMiss;
    X_i.DirectContribution = E_direct;
    X_i.BxdfEval = bxdfEval;
    X_i.Jacobian = 1.0f;
}

void ReEvaluate(inout CandidateGI X_i, ContextPrimaryRay context)
{
    if (all(X_i.DestPosition == context.HitInfo.HitPos))
    {
        X_i.BxdfEval.Radiance = 0.0f; // TODO: IsValid flag in reservoir?
        return;
    }

    float3 wi = X_i.IsMiss ? X_i.DestPosition : normalize(X_i.DestPosition - context.HitInfo.HitPos);

    bool swappedSidesOfSurface = dot(X_i.DestNg, wi) >= 0.0f;
    if (swappedSidesOfSurface)
    {
        X_i.BxdfEval.Radiance = 0.0f;
        // TODO: Could possibly allow this to still be valid, but would need NEE re-evaluation which is annoying
        return;
    }

    X_i.BxdfEval = BxDF::Evaluate(context.HitInfo, context.Wo, wi, LobeTag::eAll);

    float3 v1PosMain = X_i.SourcePositionBiased;
    float3 v1PosShift = context.HitInfo.HitPosBiased;

    if (all(v1PosMain == v1PosShift))
    {
        X_i.Jacobian = 1.0f;
        return;
    }

    float3 v1NormalShift = context.HitInfo.Ng_ff;

    ShiftResult shiftResult;
    if (X_i.IsMiss)
    {
        shiftResult = ShiftReconnectEnvironment(v1NormalShift, wi);
    }
    else
    {
        float3 v2Pos = X_i.DestPosition;
        float3 v2Normal = dot(X_i.DestNg, wi) >= 0.0f ? -X_i.DestNg : X_i.DestNg; // Recompute face-forward

        shiftResult = ShiftReconnect(v1PosMain, v1PosShift, v1NormalShift, v2Pos, v2Normal);
    }

    if (!shiftResult.IsSuccessful)
    {
        X_i.BxdfEval.Radiance = 0.0f; // TODO: IsValid flag in reservoir?
        return;
    }

    X_i.Jacobian = shiftResult.Jacobian;
}

float Target(CandidateGI X_i, ContextPrimaryRay context)
{
    if (!X_i.IsMiss && all(X_i.DestPosition == context.HitInfo.HitPos))
        return 0.0f;

    float3 wi = X_i.IsMiss ? X_i.DestPosition : normalize(X_i.DestPosition - context.HitInfo.HitPos);
    float NdL = max(0.0f, dot(context.HitInfo.Ns_ff, wi));
    if (NdL <= 0 || X_i.Jacobian <= 0.0f)
        return 0.0f;

    if (X_i.BxdfEval.PDF < EPSILON || length(X_i.BxdfEval.Radiance) <= 0.0f || length(X_i.DirectContribution) <= 0.0f)
        return 0.0f;

    if (FEAT_CORE(RestirTargetVisibility))
    {
        bool occluded;
        if (X_i.IsMiss)
            TestVisibilityDirection(context.HitInfo.HitPosBiased, X_i.DestPosition, occluded);
        else
            TestVisibilityPoints(context.HitInfo.HitPosBiased, X_i.DestPosition, occluded);
        if (occluded)
            return 0.0f;
    }

    float3 beta = 1.0f;

    // Primary Hit BxDF Sample
    float3 E_indirect = X_i.BxdfEval.Radiance * abs(NdL);
    beta *= E_indirect;

    // NEE Sample
    float3 Lo = beta * X_i.DirectContribution * X_i.Jacobian;

    return Luminance(Lo);
}

float PDF(CandidateGI X_i, ContextPrimaryRay context)
{
    return X_i.BxdfEval.PDF;
}

bool TrySampleReservoirGI(
    HitInfo hitInfo,
    float3 beta,
    uint2 pixelCoord,
    float3 wo,
    out CandidateGI Y,
    out float3 Lo)
{
    Lo = 0.0f;

    uint reservoirIdx = GetReservoirBufferIndex_Final(pixelCoord);
    ReservoirGI reservoir = gReservoirBufferGI[reservoirIdx];
    Y = reservoir.Y;

    float3 wi = Y.IsMiss ? Y.DestPosition : normalize(Y.DestPosition - hitInfo.HitPos);

    // Debug
    {
        DBG_OUTPUT3(Palette(reservoirIdx),          RESTIR_ReservoirIdx);
        DBG_OUTPUT1(reservoir.WeightSum,            RESTIR_ReservoirWeightSum);
        DBG_OUTPUT1(reservoir.Confidence / 30,      RESTIR_ReservoirConfidence);
        DBG_OUTPUT1(max(0, dot(wi, hitInfo.Ns_ff)), RESTIR_NdL);
        DBG_OUTPUT1(reservoir.W_Y,                  RESTIR_W_Y);

        DBG_OUTPUT3(wi,                             RESTIRGI_Dir);
        DBG_OUTPUT3(Y.DirectContribution,           RESTIRGI_DirectContrib);
        DBG_OUTPUT3(Y.BxdfEval.Radiance,            RESTIRGI_BxdfRadiance);
        DBG_OUTPUT1(Y.BxdfEval.PDF,                 RESTIRGI_BxdfPDF);
        DBG_OUTPUT1(Y.Jacobian,                     RESTIRGI_Jacobian);

        DBG_ASSERT_GE(reservoir.W_Y, 0.0f,          RESTIR_W_Y_RANGE);
        DBG_ASSERT_GE(reservoir.WeightSum, 0.0f,    RESTIR_WEIGHT_SUM_RANGE);
    }

    if (!Y.IsMiss && all(Y.DestPosition == hitInfo.HitPos))
        return false;

    if (reservoir.Confidence <= 0.0f || reservoir.W_Y <= 0.0f)
        return false;

    if (Y.IsMiss)
        return false;

    float NdL = max(0, dot(wi, hitInfo.Ns_ff));
    if (NdL <= 0 || length(Y.BxdfEval.Radiance) <= 0.0f || Y.Jacobian <= 0.0f || length(Y.DirectContribution) <= 0.0f)
        return false;

    bool occluded;
    if (Y.IsMiss)
        TestVisibilityDirection(hitInfo.HitPosBiased, Y.DestPosition, occluded);
    else
        TestVisibilityPoints(hitInfo.HitPosBiased, Y.DestPosition, occluded);
    if (occluded)
        return 0.0f;

    // TODO: Do I need to recompute this rather than just use reservoir.PHat?

    // Primary Hit BxDF Sample
    float3 E_indirect = Y.BxdfEval.Radiance * abs(NdL) * reservoir.W_Y;
    beta *= E_indirect;

    // NEE Sample
    Lo = beta * Y.DirectContribution * Y.Jacobian;

    return true;
}

#else

void Generate(inout RngInfo rngInfo, ContextPrimaryRay context, out CandidateGI X_i) { X_i = (CandidateGI)0; }
void ReEvaluate(inout CandidateGI X_i, ContextPrimaryRay context) {}
float Target(CandidateGI X_i, ContextPrimaryRay context) { return NAN; }
float PDF(CandidateGI X_i, ContextPrimaryRay context) { return NAN; }
bool TrySampleReservoirGI(
    HitInfo hitInfo,
    float3 beta,
    uint2 pixelCoord,
    float3 wo,
    out CandidateGI Y,
    out float3 Lo) { Y = (CandidateGI)0; Lo = NAN; return NAN; }

#endif

#endif