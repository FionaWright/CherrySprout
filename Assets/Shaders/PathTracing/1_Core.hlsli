#ifndef H_CORE_H
#define H_CORE_H

#include "PathTracing/Buffers.hlsli"
#include "PathTracing/Debug/Internal/OutputColorMacros.hlsli"

#include "PathTracing/2_SamplePath.hlsli"
#include "PathTracing/2_Accumulate.hlsli"
#include "PathTracing/GradientDomain/SampleGradients.hlsli"
#include "PathTracing/GradientDomain/SetTexGradients.hlsli"
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

    if (gSettings.IsMaxFramesReached)
    {
        gTexPrimal[pixelCoord].rgb = gTexAccumulation[pixelCoord].rgb;
        return;
    }

    float3 origin = gSettings.CameraPositionWorld;

    float3 primalSum = float3(0,0,0);
	Gradients gradientsSum = (Gradients)0;

    for (uint i = 0; i < gSettings.SPP; i++)
    {
        const RngInfo rngInfo = InitializeRngInfo(pixelCoord, i, gSettings.FrameIdx, gSettings.Seed);
        DBG_OUTPUT1(Rand01_Const(rngInfo), RNG);

        PathSample pathSample = SamplePath(rngInfo, pixelCoord);
        primalSum += pathSample.Lo;

        DBG_OUTPUT3(pathSample.Lo, PathSampleLo);

		if (FEAT_CORE(GradientDomain))
		{
            Gradients gradients = SampleGradients(rngInfo, pixelCoord, pathSample);

            gradientsSum.XForward += gradients.XForward;
            gradientsSum.XBackward += gradients.XBackward;
            gradientsSum.YForward += gradients.YForward;
            gradientsSum.YBackward += gradients.YBackward;
		}
    }

    primalSum /= float(gSettings.SPP);

    if (!FEAT_CORE(ScreenSpaceGradients)) // TODO: Ugly. Remove when SS gradients not needed? Unsure
        DBG_OUTPUT_SET(primalSum);

    float3 average = AccumulateAndFetch(pixelCoord, primalSum);
    DBG_PATH_DUMP_HIGHLIGHT(average);

    gTexPrimal[pixelCoord].rgb = average;

    if (FEAT_CORE(GradientDomain))
        SetTexGradients(pixelCoord, gradientsSum);

    DBG_OUTPUT_SET_IF_FOUND(gTexPrimal[pixelCoord].rgb);
}

#endif