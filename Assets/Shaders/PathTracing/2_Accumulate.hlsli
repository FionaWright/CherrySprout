#ifndef H_ACCUMULATE_H
#define H_ACCUMULATE_H

#include "PathTracing/Buffers.hlsli"
#include "PathTracing/Flags/MethodsHlsl.hlsli"

#include "Utils/Debug/NaNTests.hlsli"
#include "Utils/Constants.h"

float3 AccumulateAndFetch(uint2 pixelCoord, float3 primal)
{
    bool isAccumUninit = gSettings.FrameIdx == 0 || !FEATURE_ENABLED(Accumulation);
    float3 accumColor = isAccumUninit ? 0 : gTexAccumulation.Load(pixelCoord).rgb;

    // Instead of averaging samples, replace the accum value whenever a valid one is found
    // Useful for when chosen ray depth > 0 and many debug outputs will often return NAN
    if (DEBUG_ENABLED(OutputColor) && DEBUG_ENABLED(OutputColorFindAny))
    {
        if (isAccumUninit || (IsNaN3(accumColor) && IsNaN3(primal)))
        {
            gTexAccumulation[pixelCoord].rgb = NAN;
            return GetNaNVisualizerColor(pixelCoord, gSettings.FrameIdx, gSettings.FrameDimensions);
        }

        if (IsNaN3(primal))
            return accumColor;

        gTexAccumulation[pixelCoord].rgb = primal;
        if (!IsNaN3(accumColor))
            primal = (primal + accumColor) / 2.0f; // We can't average over the total number of samples as accumFrameCount is different per-pixel, so we average 2 samples
        return primal;
    }

    if (!DEBUG_ENABLED(OutputColor))
        DBG_ASSERT_VALUE(primal, COLOR_OUT);

    if (DEBUG_ENABLED(NaNTests))
    {
        if (IsNaN3(primal) || IsNaN3(accumColor))
        {
            if (FEATURE_ENABLED(Accumulation))
                gTexAccumulation[pixelCoord].rgb = NAN;
            return GetNaNVisualizerColor(pixelCoord, gSettings.FrameIdx, gSettings.FrameDimensions);
        }
        else if (IsInf3(primal) || IsInf3(accumColor))
        {
            if (FEATURE_ENABLED(Accumulation))
                gTexAccumulation[pixelCoord].rgb = INF;
            return GetInfVisualizerColor(pixelCoord, gSettings.FrameIdx, gSettings.FrameDimensions);
        }
    }

    if (!FEATURE_ENABLED(Accumulation))
        return primal;

    float accumFrameCount = (float)gSettings.FrameIdx;
    float totalFrames = accumFrameCount + 1.0f;

    float3 average = (accumColor * accumFrameCount + primal) / totalFrames;
    gTexAccumulation[pixelCoord].rgb = average;

    return average;
}

void AccumulateGradients(uint2 pixelCoord, float3 gradientX, float3 gradientY)
{
    if (!FEATURE_ENABLED(Accumulation) || gSettings.FrameIdx == 0)
    {
        gTexGradientX[pixelCoord].rgb = gradientX;
        gTexGradientY[pixelCoord].rgb = gradientY;
        return;
    }

    float3 accumColorX = gTexGradientX.Load(pixelCoord).rgb;
    float3 accumColorY = gTexGradientY.Load(pixelCoord).rgb;

    float accumFrameCount = (float)gSettings.FrameIdx;
    float totalFrames = accumFrameCount + 1.0f;

    float3 averageX = (accumColorX * accumFrameCount + gradientX) / totalFrames;
    float3 averageY = (accumColorY * accumFrameCount + gradientY) / totalFrames;
    gTexGradientX[pixelCoord].rgb = averageX;
    gTexGradientY[pixelCoord].rgb = averageY;
}

#endif