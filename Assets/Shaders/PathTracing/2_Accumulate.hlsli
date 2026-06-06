#ifndef H_ACCUMULATE_H
#define H_ACCUMULATE_H

#include "PathTracing/Buffers.hlsli"
#include "PathTracing/Flags.h"

#include "Utils/HlslUtils.hlsli"
#include "Utils/Constants.h"

float3 AccumulateAndFetch(uint2 pixelCoord, float3 color, bool nanTestEnabled)
{
    float3 accumColor = gTexAccumulation.Load(pixelCoord).rgb;

    bool isNaN = nanTestEnabled && (IsNaN3(color) || IsNaN3(accumColor));
    if (isNaN)
    {
        color = GetNaNVisualizerColor(pixelCoord);
        gTexAccumulation[pixelCoord].rgb = NAN;
        return color;
    }

    if (!FEATURE_ENABLED(Accumulation))
        return color;

    float3 newSum = accumColor + color;

    float accumFrameCount = (float)gSettings.FrameIdx;
    float totalFrames = accumFrameCount + 1.0f;

    float3 average = (accumColor * accumFrameCount + color) / totalFrames;
    gTexAccumulation[pixelCoord].rgb = average;

    return average;
}

#endif