#ifndef H_SAMPLE_PATH_H
#define H_SAMPLE_PATH_H

#include "PathTracing/RandomSample.hlsli"
#include "PathTracing/3_GetPrimaryRay.hlsli"
#include "PathTracing/3_TracePath.hlsli"
#include "PathTracing/Shift/EvaluateSubPath.hlsli"

PathSample SamplePath(RngInfo rngInfo, uint2 pixelCoord)
{
    RngValuesPrimary rngPrimary = SampleRngPrimary(rngInfo);

    float3 rayOrigin;
    float3 rayDirection;
    GetPrimaryRay(
        rngPrimary, gSettings.CameraPositionWorld, gSettings.TexelSize,
        pixelCoord, gSettings.InvV, gSettings.InvP,
        gSettings.DofFocalDist, gSettings.DofLensRadius,
        rayOrigin, rayDirection);

    PathSample pathSample = TracePath(rayOrigin, rayDirection, rngInfo, pixelCoord);
    DBG_SCALE_INTENSITY_GLOBAL(pathSample.Lo);

    pathSample.RngPrimary = rngPrimary;

    if (FEAT_DBG(EvaluatePathTest))
    {
        uint i = 1;

        uint numSubPaths = pathSample.VertexList.NumVertices;
        LobeTag cachedLobe = pathSample.VertexList.Array[i].Lobe;

        bool isNeeEndPoint = i < numSubPaths - 1;
        if (isNeeEndPoint && pathSample.VertexList.Array[i].Type == VertexType::eStandard)
        {
            pathSample.VertexList.Array[i].Lobe = LobeTag::eNEE;
            pathSample.VertexList.NumVertices = i + 1;
        }
        else if (i != UINT_MAX)
        {
            pathSample.Lo = 0.0f;
            return pathSample;
        }

        float3 Lo;
        float pdf;
        float finalMIS;
        EvaluateSubPath(pathSample.VertexList, Lo, pdf, finalMIS);

        pathSample.Lo = Lo / max(1e-6f, pdf);

        pathSample.VertexList.Array[i].Lobe = cachedLobe;
        pathSample.VertexList.NumVertices = numSubPaths;
    }

    return pathSample;
}

#endif