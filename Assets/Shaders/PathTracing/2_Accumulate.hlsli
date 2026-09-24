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

void AccumulateGradientsAndFetch(uint2 pixelCoord, Gradients gradients)
{
    if (!FEATURE_ENABLED(Accumulation) || gSettings.FrameIdx == 0)
    {
        gTexGradientXF[pixelCoord].rgb = gradients.XForward;
        gTexGradientXB[pixelCoord].rgb = gradients.XBackward;
        gTexGradientYF[pixelCoord].rgb = gradients.YForward;
        gTexGradientYB[pixelCoord].rgb = gradients.YBackward;

        DBG_OUTPUT3(gradients.XForward, GD_GradientXF);
        DBG_OUTPUT3(gradients.XBackward, GD_GradientXB);
        DBG_OUTPUT3(gradients.YForward, GD_GradientYF);
        DBG_OUTPUT3(gradients.YBackward, GD_GradientYB);
        return;
    }

    float3 accumColorXF = gTexGradientXF.Load(pixelCoord).rgb;
    float3 accumColorXB = gTexGradientXB.Load(pixelCoord).rgb;
    float3 accumColorYF = gTexGradientYF.Load(pixelCoord).rgb;
    float3 accumColorYB = gTexGradientYB.Load(pixelCoord).rgb;

    float accumFrameCount = (float)gSettings.FrameIdx;
    float totalFrames = accumFrameCount + 1.0f;

    float3 averageXF = (accumColorXF * accumFrameCount + gradients.XForward) / totalFrames;
    float3 averageXB = (accumColorXB * accumFrameCount + gradients.XBackward) / totalFrames;
    float3 averageYF = (accumColorYF * accumFrameCount + gradients.YForward) / totalFrames;
    float3 averageYB = (accumColorYB * accumFrameCount + gradients.YBackward) / totalFrames;

    gTexGradientXF[pixelCoord].rgb = averageXF;
    gTexGradientXB[pixelCoord].rgb = averageXB;
    gTexGradientYF[pixelCoord].rgb = averageYF;
    gTexGradientYB[pixelCoord].rgb = averageYB;

    DBG_OUTPUT3(averageXF, GD_GradientXF);
    DBG_OUTPUT3(averageXB, GD_GradientXB);
    DBG_OUTPUT3(averageYF, GD_GradientYF);
    DBG_OUTPUT3(averageYB, GD_GradientYB);
}

#endif