#ifndef H_CORE_H
#define H_CORE_H

#include "PathTracing/Buffers.hlsli"
#include "PathTracing/2_GetPrimaryRay.hlsli"
#include "PathTracing/2_Trace.hlsli"
#include "PathTracing/2_Accumulate.hlsli"

#include "Utils/Random.h"
#include "Utils/HlslUtils.hlsli"

void Core(uint2 pixelCoord)
{
    if (gSettings.IsMaxFramesReached) // TODO: Make this cleaner?
    {
        float3 average = gTexAccumulation[pixelCoord].rgb;
        average = pow(average, 1.0f/2.2f);
        gTexOutput[pixelCoord].rgb = average;
        return;
    }

    float3 origin = gSettings.CameraPositionWorld;

    float3 colorSum = float3(0,0,0);
    for (uint i = 0; i < gSettings.SPP; i++)
    {
        RngInfo rngInfo = InitializeRngInfo(pixelCoord, i, gSettings.FrameIdx);

        DBG_OUTPUT1(Rand01(rngInfo), RNG);

        RayDesc ray;
        ray.TMin = 0.001;
        ray.TMax = 1000.0;

        GetPrimaryRay(
            rngInfo, gSettings.CameraPositionWorld, gSettings.TexelSize,
            pixelCoord, gSettings.InvV, gSettings.InvP,
            gSettings.DofFocalDist, gSettings.DofLensRadius,
            ray.Origin, ray.Direction);

        colorSum += Trace(ray, rngInfo, pixelCoord);
    }

    colorSum /= float(gSettings.SPP);

    DBG_OUTPUT_SET(colorSum);

    float3 average = AccumulateAndFetch(pixelCoord, colorSum);

    // TODO: Better gamma correction
    if (true)
        average = pow(average, 1.0f/2.2f);

    gTexOutput[pixelCoord].rgb = average;
}

#endif