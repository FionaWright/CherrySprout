#ifndef H_CORE_H
#define H_CORE_H

#include "PathTracing/Buffers.hlsli"
#include "PathTracing/2_SamplePath.hlsli"
#include "PathTracing/2_Accumulate.hlsli"
#include "PathTracing/GradientDomain/SampleGradients.hlsli"
#include "PathTracing/Utils.hlsli"

#include "PathTracing/Debug/Globals.hlsli"
#include "PathTracing/Debug/Assert.hlsli"
#include "PathTracing/Debug/Scales.hlsli"

#include "Utils/Random.h"
#include "Utils/HlslUtils.hlsli"

void Core(uint2 pixelCoord)
{
    DBG_SET_PIXEL_INFO(pixelCoord, gSettings.FrameIdx);
    DBG_PATH_DUMP_CLEAR();

    if (gSettings.IsMaxFramesReached) // TODO: Make this cleaner?
    {
        float3 average = gTexAccumulation[pixelCoord].rgb;
        average = pow(average, 1.0f/2.2f);
        gTexPrimal[pixelCoord].rgb = average;
        return;
    }

    float3 origin = gSettings.CameraPositionWorld;

    float3 primalSum = float3(0,0,0);
	float3 gradientSum = float3(0,0,0);

    for (uint i = 0; i < gSettings.SPP; i++)
    {
        const RngInfo rngInfo = InitializeRngInfo(pixelCoord, i, gSettings.FrameIdx);
        DBG_OUTPUT1(Rand01_Const(rngInfo), RNG);

        PathSample pathSample = SamplePath(rngInfo, pixelCoord);

		if (FEATURE_ENABLED(GradientDomain))
		{
            float3 gradient = SampleGradients(rngInfo, pixelCoord, pathSample);
            gradientSum += gradient;
		}

        primalSum += pathSample.Lo;
    }

    primalSum /= float(gSettings.SPP);

    if (FEATURE_ENABLED(GradientDomain))
    {
        gradientSum /= float(gSettings.SPP * gSettings.GradientNumSamples);
        // TODO: Write to tex, and accumulate?
    }

    DBG_OUTPUT_SET(primalSum);

    float3 average = AccumulateAndFetch(pixelCoord, primalSum);
    average = LRGB_to_SRGB(average);

    DBG_PATH_DUMP_HIGHLIGHT(average);

    gTexPrimal[pixelCoord].rgb = average;
}

#endif