#ifndef H_SAMPLE_LIGHT_H
#define H_SAMPLE_LIGHT_H

#include "Scene/PunctualLight.h"
#include "Utils/Math/Punctual.hlsli"
#include "PathTracing/MIS.hlsli"
#include "PathTracing/Debug/Scales.hlsli"

#include "PathTracing/NEE/TraceShadowRay.hlsli"
#include "PathTracing/NEE/LSD/SampleLSD.hlsli"

LightSample SampleLight(
    inout RngInfo rngInfo,
    HitInfo hitInfo,
    PathState pathState,
    BxDF bxdf,

    float3 wo)
{
    uint lightIdx;
    float pdf_lsd;
    SampleLSD(rngInfo, lightIdx, pdf_lsd);

    LightSample lightSample;

    if (lightIdx == 0)
    {
        DBG_ASSERT_EXPR(FEATURE_ENABLED(EnvironmentMap), DISABLED_LIGHT_INDEX);

        float xi1 = Rand01(rngInfo);
        float xi2 = Rand01(rngInfo);

        float2 uv_env;
        float pdf_env;
        SampleEnvMapCdf(xi1, xi2, uv_env, lightSample.Direction, pdf_env);

        lightSample.Radiance = gTexEnvMap.Sample(gSampler, uv_env).rgb;
        DBG_SCALE_INTENSITY_ENV_MAP(lightSample.Radiance);

        lightSample.Distance = INF;
        lightSample.PDF = pdf_env * pdf_lsd;
        lightSample.IsDelta = false;
    }
    else
    {
        if (DEBUG_ENABLED(Asserts))
        {
            uint puncLightCount, _;
            gMegaBufferPunctuals.GetDimensions(puncLightCount, _);
            DBG_ASSERT_LT(lightIdx, puncLightCount+1, OOB_LIGHT_INDEX);
        }

        uint punctualLightIdx = lightIdx - 1;
        PunctualLight light = gMegaBufferPunctuals[punctualLightIdx];

        lightSample = EvaluatePunctualLight(light, hitInfo.HitPos);
        DBG_SCALE_INTENSITY_PUNCTUAL(lightSample.Radiance);

        lightSample.PDF = pdf_lsd;
        lightSample.IsDelta = true;
    }

    lightSample.Index = lightIdx;

    DBG_OUTPUT3(lightSample.Radiance, NEE_LightSampleRadiance);
    DBG_OUTPUT1(lightSample.Index, NEE_LightSampleIdx);
    DBG_OUTPUT1(lightSample.Distance, NEE_LightSampleDistance);
    DBG_OUTPUT3(lightSample.Direction, NEE_LightSampleDir);
    DBG_OUTPUT1(lightSample.PDF, NEE_LightSamplePDF);

    return lightSample;
}

LightSample EvaluateLight(
    HitInfo hitInfo,
    PathState pathState,
    BxDF bxdf,

    uint lightIdx,
    float3 wi,

    float3 wo)
{
    float pdf_lsd;
    EvaluateLSD(lightIdx, pdf_lsd);

    LightSample lightSample;

    if (lightIdx == 0)
    {
        DBG_ASSERT_EXPR(FEATURE_ENABLED(EnvironmentMap), DISABLED_LIGHT_INDEX);

        float2 uv_env = EaSphereToSquare(wi);

        lightSample.Radiance = gTexEnvMap.Sample(gSampler, uv_env).rgb;
        DBG_SCALE_INTENSITY_ENV_MAP(lightSample.Radiance);

        float pdf_env = GetEnvMapPdf(uv_env);
        lightSample.PDF = pdf_env * pdf_lsd;

        lightSample.Direction = wi;
        lightSample.Distance = INF;
        lightSample.IsDelta = false;
    }
    else
    {
        uint punctualLightIdx = lightIdx - 1;
        PunctualLight light = gMegaBufferPunctuals[punctualLightIdx];

        lightSample = EvaluatePunctualLight(light, hitInfo.HitPos);
        DBG_SCALE_INTENSITY_PUNCTUAL(lightSample.Radiance);

        lightSample.PDF = pdf_lsd;

        lightSample.IsDelta = true;
    }

    lightSample.Index = lightIdx;

    return lightSample;
}

#endif