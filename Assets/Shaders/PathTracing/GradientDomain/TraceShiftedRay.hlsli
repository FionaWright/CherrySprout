#ifndef H_TRACE_SHIFTED_RAY_H
#define H_TRACE_SHIFTED_RAY_H

enum class ReconnectionState : uint
{
    eUnconnected,
    eSemiConnected, // Connected but has different wo so needs BxDF evals
    eConnected      // Can reuse Hit contribution
};

PathSample TraceShiftedRay(
    PathVertexList mainVertices,
    float3 origin,
    float3 dir,
    inout RngInfo rngInfo,
    uint2 pixelCoord,

    out bool isSymmetric)
{
    ReconnectionState reconnectionState = ReconnectionState::eUnconnected;

    for (uint i = 0; i < mainVertices.NumVertices-1; i++)
    {
        PathVertex v1 = mainVertices.Array[i];
        PathVertex v2 = mainVertices.Array[i+1];

        if (reconnectionState == ReconnectionState::eUnconnected)
        {

        }
    }
}

#endif