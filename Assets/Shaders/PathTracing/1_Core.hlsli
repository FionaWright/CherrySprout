#ifndef H_CORE_H
#define H_CORE_H

static bool gRunBxdfTestForPixel = false;

#include "PathTracing/Buffers.hlsli"
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

    RayDesc ray;
    ray.Origin = origin;
    ray.Direction = 0;
    ray.TMin = 0.001;
    ray.TMax = 1000.0;

    float3 colorSum = float3(0,0,0);
    for (uint i = 0; i < gSettings.SPP; i++)
    {
        RngInfo rngInfo = InitializeRngInfo(pixelCoord, i, gSettings.FrameIdx);

        DBG_OUTPUT1(Rand01(rngInfo), RNG);

        float2 pixelUV = pixelCoord;
        if (FEATURE_ENABLED(Jitter))
        {
            float rJitterX = Rand01(rngInfo);
            float rJitterY = Rand01(rngInfo);
            float2 jitter = float2(rJitterX, rJitterY) - 0.5f;
            pixelUV += jitter;
        }
        pixelUV *= gSettings.TexelSize;

        if ((DEBUG_ENABLED(BxdfTestRevaluate) || DEBUG_ENABLED(BxdfTestHemisphere)) && pixelUV.x < 0.5f)
        {
            pixelUV.x = 1 - pixelUV.x;
            gRunBxdfTestForPixel = true;
        }

        float2 ndc = RemapUtoS(pixelUV);
        ndc.y = -ndc.y;
        float4 clip = float4(ndc, 0, 1); // z=0 for near plane
        float4 view = mul(gSettings.InvP, clip);
        view /= view.w;
        float4 world = mul(gSettings.InvV, view);
        ray.Direction = normalize(world.xyz - origin);

        if (FEATURE_ENABLED(DepthOfField))
        {
            float3 camRight = normalize(float3(gSettings.InvV[0][0], gSettings.InvV[1][0], gSettings.InvV[2][0]));
            float3 camUp = normalize(float3(gSettings.InvV[0][1], gSettings.InvV[1][1], gSettings.InvV[2][1]));
            float3 focalPoint = origin + ray.Direction * gSettings.DofFocalDist;

            float rLensU = Rand01(rngInfo);
            float rLensV = Rand01(rngInfo);
            float r = sqrt(rLensU) * gSettings.DofLensRadius;
            float theta = 2.0 * PI * rLensV;
            float3 lensOffset = r * (camRight * cos(theta) + camUp * sin(theta));

            ray.Origin = origin + lensOffset;
            ray.Direction = normalize(focalPoint - ray.Origin);
        }

        colorSum += Trace(ray, rngInfo);
    }

    colorSum /= float(gSettings.SPP);

    DBG_OUTPUT_SET(colorSum);

    float3 average = AccumulateAndFetch(pixelCoord, colorSum);

    // TODO: Better gamma correction?
    if (true)
        average = pow(average, 1.0f/2.2f);

    gTexOutput[pixelCoord].rgb = average;
}

#endif