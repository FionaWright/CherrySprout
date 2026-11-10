#ifndef H_SAMPLE_PATH_H
#define H_SAMPLE_PATH_H

#include "PathTracing/RandomSample.hlsli"
#include "PathTracing/3_GetPrimaryRay.hlsli"
#include "PathTracing/3_TracePath.hlsli"
#include "PathTracing/3_EvaluatePath.hlsli"

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
        uint neeIdx = 1;
        if (neeIdx != UINT_MAX)
        {
            if (neeIdx < pathSample.VertexList.NumVertices && pathSample.VertexList.Array[neeIdx].Type == VertexType::eStandard)
            {
                pathSample.VertexList.Array[neeIdx].Lobe = LobeTag::eNEE;
                pathSample.VertexList.NumVertices = neeIdx + 1;
            }
            else
            {
                pathSample.Lo = 0.0f;
                return pathSample;
            }
        }

        float3 Lo;
        float pdf;
        EvaluateSubPath(pathSample.VertexList, Lo, pdf);
        pathSample.Lo = Lo / max(1e-6f, pdf);
    }

    return pathSample;
}

#endif