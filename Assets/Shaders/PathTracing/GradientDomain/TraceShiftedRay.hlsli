#ifndef H_TRACE_SHIFTED_RAY_H
#define H_TRACE_SHIFTED_RAY_H

enum class ReconnectionState : uint
{
    eUnconnected,
    eSemiConnected,
    eConnected
};

PathSample TraceShiftedRay(
    PathVertexList mainVertices,
    float3 origin,
    float3 dir,
    inout RngInfo rngInfo,
    uint2 pixelCoord,

    out bool isSymmetric)
{
    isSymmetric = false;
    return (PathSample)0;
}

#endif