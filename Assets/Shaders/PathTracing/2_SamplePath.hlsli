#ifndef H_SAMPLE_PATH_H
#define H_SAMPLE_PATH_H

#include "PathTracing/RandomSample.hlsli"
#include "PathTracing/3_GetPrimaryRay.hlsli"
#include "PathTracing/3_TracePath.hlsli"

PathSample SamplePath(RngInfo rngInfo, uint2 pixelCoord)
{
    float3 rayOrigin;
    float3 rayDirection;

    RngValuesPrimary rngPrimary = SampleRngPrimary(rngInfo);

    GetPrimaryRay(
        rngPrimary, gSettings.CameraPositionWorld, gSettings.TexelSize,
        pixelCoord, gSettings.InvV, gSettings.InvP,
        gSettings.DofFocalDist, gSettings.DofLensRadius,
        rayOrigin, rayDirection);

    PathSample pathSample = TracePath(rayOrigin, rayDirection, rngInfo, pixelCoord);
    DBG_SCALE_INTENSITY_GLOBAL(pathSample.Lo);

    pathSample.RngPrimary = rngPrimary;

    return pathSample;
}

#endif