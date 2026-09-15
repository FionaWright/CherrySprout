#ifndef H_TRACE_SHIFTED_RAY_H
#define H_TRACE_SHIFTED_RAY_H

#include "PathTracing/ShiftMapping/ShiftMapping.hlsli"

enum class ReconnectionState : uint
{
    eUnconnected,
    eSemiConnected, // Connected but has different wo so needs BxDF evals
    eConnected      // Can reuse Hit contribution
};

void HitShiftedUnconnected(
    inout ReconnectionState reconnectionState,
    inout bool isSymmetric,
    inout PathState pathState,

    PathVertexInfo v1,
    HitInfo hitInfo,
    float3 wo,
    float3 nextOrigin,

    out float3 wi)
{
    ShiftResult shiftResult;

    if (v1.NextVertexType == VertexType::eUninitialized) // (Path terminates on v1)
    {
        // TODO: ?
    }

    if (v1.Type == VertexType::eDiffuse && v1.NextVertexType != VertexType::eGlossy)
    {
        if (v1.NextVertexType == VertexType::eDiffuse)
        {
            shiftResult = ShiftReconnect(v1.Position, nextOrigin, v1.NextVertexPosition, v1.NextVertexNormal);
        }
        else if (v1.NextVertexType == VertexType::eEnvironment)
        {
            // TODO: This doesn't make sense to me, we already know that this will succeed
            shiftResult = ShiftReconnectEnvironment(v1.Position, v1.Wi);
        }

        reconnectionState = ReconnectionState::eSemiConnected;
    }
    else
    {
        float iorNCurrent =  hitInfo.IsEntering ? IOR_N_AIR          : hitInfo.Mat.IOR_N;
        float iorNNext =     hitInfo.IsEntering ? hitInfo.Mat.IOR_N  : IOR_N_AIR;
        float eta = iorNCurrent / iorNNext;

        shiftResult = ShiftHalfVector(v1.SFrame, hitInfo.SFrame, v1.Wi, v1.Wo, wo, v1.Eta, eta);
    }

    if (!shiftResult.IsSuccessful)
    {
        isSymmetric = false;
        // TODO: What now?
        return;
    }

    pathState.PDF *= shiftResult.Jacobian;
    wi = shiftResult.Wi;

    BxDF bxdf;
    float3 f_bxdf;
    float pdf_bxdf; // TODO: pathState.PDF
    bxdf.Evaluate(hitInfo, wo, wi, f_bxdf, pdf_bxdf);

    float NdL = dot(hitInfo.Ns_ff, wi);
    pathState.Beta *= f_bxdf * abs(NdL) / max(1e-6, pdf_bxdf);

    pathState.LastBxdfPdf = pdf_bxdf;

    // TODO: Direct lighting
}

void TraceShiftedPath(
    PathVertexList mainVertices,
    float3 origin,
    float3 dir,
    inout RngInfo rngInfo,
    uint2 pixelCoord,

    out float3 Lo,
    out float pdf,
    out bool isSymmetric)
{
    RayQuery<RAY_FLAGS> q;

    ReconnectionState reconnectionState = ReconnectionState::eUnconnected;
    isSymmetric = true;

    PathState pathState = CreatePathState(origin, dir);

    for (uint i = 0; i < mainVertices.NumVertices; i++)
    {
        PathVertexInfo v1 = mainVertices.Array[i];

        pathState.RaySegmentIdx = i;
        pathState.LastRayWasDiracDelta = false;

        bool isMiss;
        HitInfo hitInfo;
        ComputeRayHit(q, pixelCoord, pathState, isMiss, hitInfo);

        if (isMiss)
        {
            // TODO: Value can be cached if connected (likely)
            float3 Li = pathState.Beta * Miss(pathState, i);
            pathState.Lo += ApplyFireflyThreshold(Li);
            break;
        }

        float3 L_sample = pathState.Beta * hitInfo.Emission;

        float3 wo = -pathState.Desc.Direction;
        float3 wi;

        float3 hitPos = pathState.Desc.Origin + pathState.Desc.Direction * hitInfo.RayT;
        float3 nextOrigin = hitPos + hitInfo.Ng_ff * EPSILON;

        if (reconnectionState == ReconnectionState::eConnected)
        {
            wi = v1.Wi;
            pathState.Beta *= v1.IndirectContribution;
            // TODO: pathState.PDF

            if (FEATURE_ENABLED(NEE))
                pathState.LastBxdfPdf = v1.BxdfPdf;
        }
        else if (reconnectionState == ReconnectionState::eSemiConnected)
        {
            wi = v1.Wi;

            BxDF bxdf;
            float3 f_bxdf;
            float pdf_bxdf; // TODO: pathState.PDF
            bxdf.Evaluate(hitInfo, wo, wi, f_bxdf, pdf_bxdf);

            float NdL = dot(hitInfo.Ns_ff, wi);
            pathState.Beta *= f_bxdf * abs(NdL) / max(1e-6, pdf_bxdf);

            if (FEATURE_ENABLED(NEE))
                pathState.LastBxdfPdf = pdf_bxdf;

            reconnectionState = ReconnectionState::eConnected;
        }
        else if (reconnectionState == ReconnectionState::eUnconnected)
        {
            HitShiftedUnconnected(reconnectionState, isSymmetric, pathState, v1, hitInfo, wo, nextOrigin, wi);
        }

        pathState.Lo += ApplyFireflyThreshold(L_sample);

        pathState.Desc.Direction = wi;
        pathState.Desc.Origin = nextOrigin;
    }

    Lo = pathState.Lo;
    pdf = pathState.PDF;
}

#endif