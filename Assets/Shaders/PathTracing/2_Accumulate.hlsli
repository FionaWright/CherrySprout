#ifndef H_ACCUMULATE_H
#define H_ACCUMULATE_H

#include "PathTracing/Buffers.hlsli"
#include "PathTracing/Flags/MethodsHlsl.hlsli"

#include "Utils/Debug/NaNTests.hlsli"
#include "Utils/Constants.h"

float3 AccumulateAndFetch(uint2 pixelCoord, float3 color)
{
    float3 accumColor = gSettings.FrameIdx == 0 || !FEATURE_ENABLED(Accumulation) ? 0 : gTexAccumulation.Load(pixelCoord).rgb;

    if (DEBUG_ENABLED(NaNTests))
    {
        if (IsNaN3(color) || IsNaN3(accumColor))
        {
            if (FEATURE_ENABLED(Accumulation))
                gTexAccumulation[pixelCoord].rgb = NAN;
            return GetNaNVisualizerColor(pixelCoord);
        }
        else if (IsInf3(color) || IsInf3(accumColor))
        {
            if (FEATURE_ENABLED(Accumulation))
                gTexAccumulation[pixelCoord].rgb = INF;
            return GetInfVisualizerColor(pixelCoord);
        }
    }

    if (!FEATURE_ENABLED(Accumulation))
        return color;

    float accumFrameCount = (float)gSettings.FrameIdx;
    float totalFrames = accumFrameCount + 1.0f;

    float3 average = (accumColor * accumFrameCount + color) / totalFrames;
    gTexAccumulation[pixelCoord].rgb = average;

    return average;
}

#endif