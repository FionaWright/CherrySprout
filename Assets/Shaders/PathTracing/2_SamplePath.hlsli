#ifndef H_SAMPLE_PATH_H
#define H_SAMPLE_PATH_H

#include "PathTracing/3_GetPrimaryRay.hlsli"
#include "PathTracing/3_Trace.hlsli"

PathSample SamplePath(RngInfo rngInfo, uint2 pixelCoord)
{
    float3 rayOrigin;
    float3 rayDirection;

    // TODO: Primary Ray PDF for GradientDomain. Disable Jitter for now
    GetPrimaryRay(
        rngInfo, gSettings.CameraPositionWorld, gSettings.TexelSize,
        pixelCoord, gSettings.InvV, gSettings.InvP,
        gSettings.DofFocalDist, gSettings.DofLensRadius,
        rayOrigin, rayDirection);

    PathSample pathSample = Trace(rayOrigin, rayDirection, rngInfo, pixelCoord);
    DBG_SCALE_INTENSITY_GLOBAL(pathSample.Lo);
    return pathSample;
}

#endif