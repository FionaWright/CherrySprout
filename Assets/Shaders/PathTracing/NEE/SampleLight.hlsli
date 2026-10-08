#ifndef H_SAMPLE_LIGHT_H
#define H_SAMPLE_LIGHT_H

#include "Scene/PunctualLight.h"
#include "Utils/Math/Punctual.hlsli"
#include "PathTracing/MIS.hlsli"
#include "PathTracing/Debug/Scales.hlsli"

#include "PathTracing/NEE/TraceShadowRay.hlsli"
#include "PathTracing/NEE/LSD/SampleLSD.hlsli"
#include "PathTracing/NEE/SampleEnvMap.hlsli"
#include "PathTracing/NEE/SampleEmissive.hlsli"

LightSample SampleLight(
    inout RngInfo rngInfo,
    HitInfo hitInfo,
    PathState pathState,
    BxDF bxdf,

    float3 wo)
{
    LightSample lightSample;

    if (gSettings.LsdCount == 0)
    {
        lightSample.Index = 0;
        lightSample.Radiance = 0;
        lightSample.Distance = INF;
        lightSample.PDF = 1.0f;
        return lightSample;
    }

    uint lightIdx;
    float pdf_lsd;
    SampleLSD(rngInfo, lightIdx, pdf_lsd);

    if (lightIdx < gSettings.LsdBasePunctuals)
    {
        DBG_ASSERT_EXPR(FEATURE_ENABLED(EnvironmentMap), DISABLED_LIGHT_INDEX);
        DBG_OUTPUT3(float3(1,0,0),  NEE_LightSampleType);

        lightSample = SampleEnvMap(rngInfo);
        DBG_SCALE_INTENSITY_ENV_MAP(lightSample.Radiance);

        lightSample.IsDelta = false;
    }
    else if (lightIdx < gSettings.LsdBaseEmissives)
    {
        DBG_ASSERT_EXPR(FEATURE_ENABLED(Punctuals), DISABLED_LIGHT_INDEX);
        DBG_OUTPUT3(float3(0,1,0),  NEE_LightSampleType);

        uint punctualIdx = lightIdx - gSettings.LsdBasePunctuals;

        if (DEBUG_ENABLED(Asserts))
        {
            uint count, _;
            gMegaBufferPunctuals.GetDimensions(count, _);
            DBG_ASSERT_LT(punctualIdx, count, OOB_LIGHT_INDEX);
        }

        PunctualLight light = gMegaBufferPunctuals[punctualIdx];

        lightSample = EvaluatePunctualLight(light, hitInfo.HitPos);
        DBG_SCALE_INTENSITY_PUNCTUAL(lightSample.Radiance);

        lightSample.PDF = 1.0f;
        lightSample.IsDelta = true;
    }
    else
    {
        DBG_ASSERT_EXPR(FEATURE_ENABLED(Emission), DISABLED_LIGHT_INDEX);
        DBG_ASSERT_LT(lightIdx, gSettings.LsdCount, OOB_LIGHT_INDEX);
        DBG_OUTPUT3(float3(0,0,1),  NEE_LightSampleType);

        uint emissiveIdx = lightIdx - gSettings.LsdBaseEmissives;

        lightSample = SampleEmissive(rngInfo, emissiveIdx, hitInfo);
        DBG_SCALE_INTENSITY_EMISSION(lightSample.Radiance);

        lightSample.IsDelta = false;
    }

    lightSample.Index = lightIdx;
    lightSample.PDF *= pdf_lsd;

    DBG_OUTPUT3(lightSample.Radiance,  NEE_LightSampleRadiance);
    DBG_OUTPUT1(lightSample.Index,     NEE_LightSampleIdx);
    DBG_OUTPUT1(lightSample.Distance,  NEE_LightSampleDistance);
    DBG_OUTPUT3(lightSample.Direction, NEE_LightSampleDir);
    DBG_OUTPUT1(lightSample.PDF,       NEE_LightSamplePDF);

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

    if (lightIdx < gSettings.LsdBasePunctuals)
    {
        DBG_ASSERT_EXPR(FEATURE_ENABLED(EnvironmentMap), DISABLED_LIGHT_INDEX);

        float2 uv_env = EaSphereToSquare(wi);

        lightSample.Radiance = gTexEnvMap.Sample(gSamplerLinearWrap, uv_env).rgb;
        DBG_SCALE_INTENSITY_ENV_MAP(lightSample.Radiance);

        float pdf_env = EvaluateEnvMapPdf(uv_env);
        lightSample.PDF = pdf_env;

        lightSample.Direction = wi;
        lightSample.Distance = INF;
        lightSample.IsDelta = false;
    }
    else if (lightIdx < gSettings.LsdBaseEmissives)
    {
        DBG_ASSERT_EXPR(FEATURE_ENABLED(Punctuals), DISABLED_LIGHT_INDEX);

        uint punctualIdx = lightIdx - gSettings.LsdBasePunctuals;
        PunctualLight light = gMegaBufferPunctuals[punctualIdx];

        lightSample = EvaluatePunctualLight(light, hitInfo.HitPos);
        DBG_SCALE_INTENSITY_PUNCTUAL(lightSample.Radiance);

        lightSample.PDF = 1.0f;
        lightSample.IsDelta = true;
    }
    else
    {
        DBG_ASSERT_EXPR(FEATURE_ENABLED(Emission), DISABLED_LIGHT_INDEX);
        DBG_ASSERT_LT(lightIdx, gSettings.LsdCount, OOB_LIGHT_INDEX);
        DBG_OUTPUT3(float3(0,0,1),  NEE_LightSampleType);

        uint emissiveIdx = lightIdx - gSettings.LsdBaseEmissives;

        lightSample = EvaluateEmissive(emissiveIdx, hitInfo, wi);
        DBG_SCALE_INTENSITY_EMISSION(lightSample.Radiance);

        lightSample.IsDelta = false;
    }

    lightSample.Index = lightIdx;
    lightSample.PDF *= pdf_lsd;

    return lightSample;
}

#endif