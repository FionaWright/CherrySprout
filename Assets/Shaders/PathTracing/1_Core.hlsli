#ifndef H_CORE_H
#define H_CORE_H

#include "PathTracing/Buffers.hlsli"
#include "PathTracing/Debug/Internal/OutputColorMacros.hlsli"

#include "PathTracing/2_SamplePath.hlsli"
#include "PathTracing/2_Accumulate.hlsli"
#include "PathTracing/GradientDomain/SampleGradients.hlsli"
#include "PathTracing/GradientDomain/SetTexGradients.hlsli"
#include "PathTracing/Utils.hlsli"
#include "PathTracing/ReSTIR/PT.hlsli"

#include "PathTracing/Debug/Globals.hlsli"
#include "PathTracing/Debug/Assert.hlsli"
#include "PathTracing/Debug/Scales.hlsli"
#include "PathTracing/Debug/Tests/BxdfCore.hlsli"

#include "Utils/Random.h"
#include "Utils/HlslUtils.hlsli"

void Core(uint2 pixelCoord)
{
    DBG_SET_PIXEL_INFO(pixelCoord, gSettings.FrameIdx);
    DBG_PATH_DUMP_CLEAR();
    DBG_OUTPUT1(0.0f, Black);

    if (FEAT_DBG(BxdfTestCore))
    {
        Test_BxdfCore(pixelCoord);
        return;
    }

    float3 primalSum = float3(0,0,0);
    Gradients gradientsSum = INIT0(Gradients);

    if (FEAT_CORE(RestirPT))
    {
        uint contextIdx = GetContextBufferIndex(pixelCoord);
        ContextPrimaryRay context = gContextsPrimaryRay[contextIdx];
        if (context.IsMiss)
        {
            float2 uv = EaSphereToSquare(-context.Wo);
            primalSum = gTexEnvMap.Sample(gSamplerLinearClamp, uv).rgb;
        }
        else
        {
            CandidatePT Y;
            float3 Lo;
            if (TrySampleReservoirPT(context.HitInfo, pixelCoord, context.Wo, Y, Lo))
                primalSum = Lo;
            else
                primalSum = float3(0, 0, 0); // TODO
        }
    }
    else
    {
        float3 origin = gSettings.CameraPositionWorld;

        for (uint i = 0; i < gSettings.SPP; i++)
        {
            DBG_FORCE_OUTPUT1(gSettings.FrameIdx);
            const RngInfo rngInfo = InitializeRngInfo(pixelCoord, i, gSettings.FrameIdx, gSettings.Seed);
            DBG_OUTPUT1(Rand01_Const(rngInfo), RNG);

            PathSample pathSample = SamplePath(rngInfo, pixelCoord);
            primalSum += pathSample.Lo;

            DBG_OUTPUT3(pathSample.Lo, PathSampleLo);

            if (FEAT_CORE(GradientDomain))
            {
                Gradients gradients = SampleGradients(pixelCoord, pathSample);

                gradientsSum.XForward += gradients.XForward;
                gradientsSum.XBackward += gradients.XBackward;
                gradientsSum.YForward += gradients.YForward;
                gradientsSum.YBackward += gradients.YBackward;
            }
        }

        primalSum /= float(gSettings.SPP);
    }

    DBG_OUTPUT_SET(primalSum);

    float3 average = AccumulateAndFetch(pixelCoord, primalSum);
    DBG_PATH_DUMP_HIGHLIGHT(average);

    gTexPrimal[pixelCoord].rgb = average;

    if (FEAT_CORE(GradientDomain))
        SetTexGradients(pixelCoord, gradientsSum);

    DBG_OUTPUT_SET_IF_FOUND(gTexPrimal[pixelCoord].rgb);
}

#endif