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

/*

Sample a random direction, trace the point, then sample a random light and get its direct contribution

When sampling the reservoir, does the main path continue from the sample point or does it continue from primary ray hit?

*/

CandidateGI Generate(inout RngInfo rngInfo, ContextGI context)
{
    BxDF bxdf;
    BxdfEval bxdfEval;
    BxdfSample bxdfSample = bxdf.Sample(rngInfo, context.HitInfo, context.Wo, bxdfEval);

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

    // TODO: Does SampleNEE PDF need to be accounted for in reservoir?
    float3 E_direct = 0.0f;
    if (!isMiss)
    {
        float3 wo = -bxdfSample.Wi;
        E_direct = SampleNEE(rngInfo, secondaryHitInfo, bxdf, 1, wo);
    }

    // TODO: Extension: Missed ray candidates

    CandidateGI X_i;
    X_i.SourcePositionBiased = context.HitInfo.HitPosBiased;
    X_i.DestPosition = isMiss ? 0 : secondaryHitInfo.HitPos;
    X_i.Ns_ff = isMiss ? 0 : secondaryHitInfo.Ns_ff;
    X_i.Ng_ff = isMiss ? 0 : secondaryHitInfo.Ng_ff;
    X_i.IsMiss = isMiss;
    X_i.DirectContribution = E_direct;
    X_i.BxdfEval = bxdfEval;
    return X_i;
}

void ReEvaluate(inout CandidateGI X_i, ContextGI context)
{
    float3 wi = normalize(X_i.DestPosition - context.HitInfo.HitPos);

    BxDF bxdf;
    X_i.BxdfEval = bxdf.Evaluate(context.HitInfo, context.Wo, wi);

    // TODO: Jacobian here?
}

float Target(CandidateGI X_i, ContextGI context)
{
    if (X_i.IsMiss) // TODO
        return 0.0f;

    float3 wi = normalize(X_i.DestPosition - context.HitInfo.HitPos);
    float NdL = max(0.0f, dot(context.HitInfo.Ns_ff, wi));
    if (NdL <= 0)
        return 0.0f;

    if (X_i.BxdfEval.PDF < EPSILON || length(X_i.BxdfEval.Radiance) <= 0.0f || length(X_i.DirectContribution) <= 0.0f)
        return 0.0f;

    float3 v1PosMain = X_i.SourcePositionBiased;
    float3 v1PosShift = context.HitInfo.HitPosBiased;
    float3 v1NormalShift = context.HitInfo.Ns_ff;
    float3 v2Pos = X_i.DestPosition;
    float3 v2Normal = X_i.Ns_ff;
    ShiftResult shiftResult = ShiftReconnect(v1PosMain, v1PosShift, v1NormalShift, v2Pos, v2Normal);

    if (!shiftResult.IsSuccessful)
        return 0.0f;

    // TODO: What do I do with shiftResult.Jacobian?

    float3 beta = 1.0f;

    // Primary Hit BxDF Sample
    E_indirect = bxdfEval.Radiance * abs(NdL);

    // NEE Sample
    float3 Lo = beta * X_i.DirectContribution;

    return Luminance(Lo);
}

float PDF(CandidateGI X_i, ContextGI context)
{
    return X_i.BxdfEval.PDF;
}

bool TrySampleReservoirGI(
    HitInfo hitInfo,
    float3 beta,
    uint2 pixelCoord,
    float3 wo,
    BxDF bxdf,
    uint numSamples,
    out CandidateGI Y,
    out float3 Lo)
{
    Lo = 0.0f;

    uint reservoirIdx = GetReservoirBufferIndex_Final(pixelCoord);
    ReservoirDI reservoir = gReservoirBuffer[reservoirIdx];

    if (reservoir.Confidence <= 0.0f || reservoir.W_Y <= 0.0f)
        return false;

    Y = reservoir.Y;
    float3 wi = normalize(Y.DestPosition - hitInfo.HitPos);
    BxdfEval bxdfEval = Y.BxdfEval;

    float NdL = max(0, dot(wi, hitInfo.Ns_ff));
    if (NdL <= 0 || length(bxdfEval.Radiance) <= 0.0f)
        return false;

    // Primary Hit BxDF Sample
    float3 E_indirect = bxdfEval.Radiance * abs(NdL) * reservoir.W_Y;
    beta *= E_indirect;

    // NEE Sample
    Lo = beta * Y.DirectContribution;

    return true;
}

#else

CandidateGI Generate(inout RngInfo rngInfo, ContextGI context) { return (CandidateGI)0; }
float Target(CandidateGI X_i, ContextGI context) { return NAN; }
float PDF(CandidateGI X_i, ContextGI context) { return NAN; }
bool TrySampleReservoirGI(
    HitInfo hitInfo,
    float3 beta,
    uint2 pixelCoord,
    float3 wo,
    BxDF bxdf,
    uint numSamples,
    out CandidateGI Y,
    out float3 Lo) { Y = (CandidateGI)0; Lo = NAN; return NAN; }

#endif

#endif