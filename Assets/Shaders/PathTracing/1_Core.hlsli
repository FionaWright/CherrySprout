#ifndef H_CORE_H
#define H_CORE_H

#include "PathTracing/Buffers.hlsli"
#include "PathTracing/2_Trace.hlsli"
#include "PathTracing/2_Accumulate.hlsli"

#include "Utils/Random.h"
#include "Utils/HlslUtils.hlsli"

void Core(uint2 pixelCoord)
{
    if (gSettings.MaxFramesReached) // TODO: Make this cleaner?
    {
        float3 average = gTexAccumulation[pixelCoord].rgb;
        average = pow(average, 1.0f/2.2f);
        gTexOutput[pixelCoord].rgb = average;
        return;
    }

    float3 origin = gSettings.CameraPositionWorld;

    RayDesc ray;
    ray.Origin = origin;
    ray.Direction = 0;
    ray.TMin = 0.001;
    ray.TMax = 1000.0;

    float3 colorSum = float3(0,0,0);
    for (uint i = 0; i < gSettings.SPP; i++)
    {
        RngInfo rngInfo = InitializeRngInfo(pixelCoord, i, gSettings.FrameIdx);

        float2 pixelUV = pixelCoord;
        if (FEATURE_ENABLED(Jitter))
        {
            float rJitterX = Rand01(rngInfo);
            float rJitterY = Rand01(rngInfo);
            float2 jitter = float2(rJitterX, rJitterY) - 0.5f;
            pixelUV += jitter;
        }
        pixelUV *= gSettings.TexelSize;

        //float2 ndc = RemapUtoS(pixelUV);
        float2 ndc = pixelUV * 2.0f - 1.0f;
        ndc.y = -ndc.y;
        float4 clip = float4(ndc, 0, 1); // z=0 for near plane
        float4 view = mul(gSettings.InvP, clip);
        view /= view.w;
        float4 world = mul(gSettings.InvV, view);
        ray.Direction = normalize(world.xyz - origin);

        colorSum += Trace(ray, rngInfo);
    }

    colorSum /= float(gSettings.SPP);

    float3 average = AccumulateAndFetch(pixelCoord, colorSum);

    // TODO: Better gamma correction?
    if (true)
        average = pow(average, 1.0f/2.2f);

    gTexOutput[pixelCoord].rgb = average;
}

#endif