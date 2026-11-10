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
        float3 Lo;
        float pdf;
        EvaluatePath(pathSample.VertexList, Lo, pdf);
        pathSample.Lo = Lo / max(1e-6f, pdf);
    }

    return pathSample;
}

#endif