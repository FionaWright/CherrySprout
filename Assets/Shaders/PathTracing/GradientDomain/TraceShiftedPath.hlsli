#ifndef H_TRACE_SHIFTED_RAY_H
#define H_TRACE_SHIFTED_RAY_H

enum class ReconnectionState : uint
{
    eUnconnected,
    eSemiConnected, // Connected but has different wo so needs BxDF evals
    eConnected      // Can reuse Hit contribution
};

void HitShiftedConnected()
{
}

void HitShiftedSemiConnected()
{
}

void HitShiftedUnconnected(
    inout ReconnectionState reconnectionState,
    inout bool isSymmetric,
    inout PathState pathState,

    PathVertex v1,
    PathVertex v2,
    HitInfo hitInfo,
    float3 wo,

    out float3 wi)
{
    if (v1.IsDiffuse && v2.IsDiffuse)
    {
        // ReconnectionShift
        reconnectionState = ReconnectionState::eSemiConnected;
        continue;
    }

    float iorNCurrent =  hitInfo.IsEntering ? IOR_N_AIR          : hitInfo.Mat.IOR_N;
    float iorNNext =     hitInfo.IsEntering ? hitInfo.Mat.IOR_N  : IOR_N_AIR;
    float eta = nCurrent / nNext;

    ShiftResult shiftResult = ShiftHalfVector(v1.SFrame, hitInfo.SFrame, v1.Wi, v1.Wo, wo, v1.Eta, eta);

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
    bxdf.Evaluate(hitInfo, wo, shiftResult.Wi, f_bxdf, pdf_bxdf);

    float NdL = dot(hitInfo.Ns_ff, wi);
    pathState.Beta *= f_bxdf * abs(NdL) / max(1e-6, pdf_bxdf);

    pathState.LastBxdfPdf = pdf_bxdf;

    // TODO: Direct lighting
}

PathSample TraceShiftedPath(
    PathVertexList mainVertices,
    float3 origin,
    float3 dir,
    inout RngInfo rngInfo,
    uint2 pixelCoord,

    out bool isSymmetric)
{
    RayQuery<RAY_FLAGS> q;

    ReconnectionState reconnectionState = ReconnectionState::eUnconnected;
    isSymmetric = true;

    PathState pathState = CreatePathState(origin, dir);

    for (uint i = 0; i < mainVertices.NumVertices-1; i++) // TODO: How to handle last vertex
    {
        PathVertex v1 = mainVertices.Array[i];
        PathVertex v2 = mainVertices.Array[i+1];

        pathState.RaySegmentIdx = i;
        pathState.LastRayWasDiracDelta = false;

        bool isMiss;
        HitInfo hitInfo;
        ComputeRayHit(q, pixelCoord, pathState, isMiss, hitInfo);

        if (isMiss)
        {
            // Note: Semi-Connected not possible here (TODO: Put assert)

            if (reconnectionState == ReconnectionState::eConnected)
            {
                
            }
        }

        float3 L_sample = pathState.Beta * hitInfo.Emission;

        float3 wo = -pathState.Desc.Direction;
        float3 wi;

        if (reconnectionState == ReconnectionState::eConnected)
        {
            pathState.Beta *= v1.IndirectContribution;
            wi = v1.Wi;
        }

        else if (reconnectionState == ReconnectionState::eSemiConnected)
        {

        }

        else if (reconnectionState == ReconnectionState::eUnconnected)
        {
            TraceShiftedRayUnconnected(reconnectionState, isSymmetric, pathState, v1, v2, hitInfo, wo, wi);
        }

        pathState.Lo += ApplyFireflyThreshold(L_sample);

        float3 hitPos = pathState.Desc.Origin + pathState.Desc.Direction * hitInfo.RayT;
        float3 nextOrigin = hitPos + hitInfo.Ng_ff * EPSILON;

        pathState.Desc.Direction = wi;
        pathState.Desc.Origin = nextOrigin;
    }
}

#endif