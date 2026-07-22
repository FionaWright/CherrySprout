#ifndef H_ACCUMULATE_H
#define H_ACCUMULATE_H

#include "PathTracing/Buffers.hlsli"
#include "PathTracing/Flags/MethodsHlsl.hlsli"

#include "Utils/Debug/NaNTests.hlsli"
#include "Utils/Constants.h"

float3 AccumulateAndFetch(uint2 pixelCoord, float3 color)
{
    bool isAccumUninit = gSettings.FrameIdx == 0 || !FEATURE_ENABLED(Accumulation);
    float3 accumColor = isAccumUninit ? 0 : gTexAccumulation.Load(pixelCoord).rgb;

    // Instead of averaging samples, replace the accum value whenever a valid one is found
    // Useful for when chosen ray depth > 0 and many debug outputs will often return NAN
    if (DEBUG_ENABLED(OutputColor) && DEBUG_ENABLED(OutputColorFindAny))
    {
        if (isAccumUninit || (IsNaN3(accumColor) && IsNaN3(color)))
        {
            gTexAccumulation[pixelCoord].rgb = NAN;
            return GetNaNVisualizerColor(pixelCoord, gSettings.FrameIdx, gSettings.FrameDimensions);
        }

        if (IsNaN3(color))
            return accumColor;

        gTexAccumulation[pixelCoord].rgb = color;
        if (!IsNaN3(accumColor))
            color = (color + accumColor) / 2.0f; // We can't average over the total number of samples as accumFrameCount is different per-pixel, so we average 2 samples
        return color;
    }

    if (!DEBUG_ENABLED(OutputColor))
        DBG_ASSERT_VALUE(color, COLOR_OUT);

    if (DEBUG_ENABLED(NaNTests))
    {
        if (IsNaN3(color) || IsNaN3(accumColor))
        {
            if (FEATURE_ENABLED(Accumulation))
                gTexAccumulation[pixelCoord].rgb = NAN;
            return GetNaNVisualizerColor(pixelCoord, gSettings.FrameIdx, gSettings.FrameDimensions);
        }
        else if (IsInf3(color) || IsInf3(accumColor))
        {
            if (FEATURE_ENABLED(Accumulation))
                gTexAccumulation[pixelCoord].rgb = INF;
            return GetInfVisualizerColor(pixelCoord, gSettings.FrameIdx, gSettings.FrameDimensions);
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