#ifndef H_SAMPLE_LIGHT_H
#define H_SAMPLE_LIGHT_H

#include "Scene/PunctualLight.h"
#include "Utils/Math/Punctual.hlsli"
#include "PathTracing/MIS.hlsli"
#include "PathTracing/Debug/Scales.hlsli"

#include "PathTracing/VisibilityRays.hlsli"
#include "PathTracing/NEE/LSD/SampleLSD.hlsli"
#include "PathTracing/NEE/SampleEnvMap.hlsli"
#include "PathTracing/NEE/SampleEmissive.hlsli"

LightSample SampleLight(
    RngValuesAncillary::RngValuesNEE rng,
    float3 hitPos,
    float3 wo,

    out LightEval lightEval)
{
    LightSample lightSample = (LightSample)0;

    if (gSettings.LsdCount == 0)
    {
        lightEval.Radiance = 0;
        lightEval.PDF = 1.0f;
        lightEval.Distance = 0;

        lightSample.Index = 0;
        return lightSample;
    }

    uint lightIdx;
    float pdf_lsd;
    SampleLSD(rng.LSD, lightIdx, pdf_lsd);

    if (lightIdx < gSettings.LsdBasePunctuals)
    {
        DBG_ASSERT_EXPR(FEAT_CORE(EnvironmentMap), DISABLED_LIGHT_INDEX);
        DBG_OUTPUT3(float3(1,0,0),  NEE_LightSampleType);

        lightSample = SampleEnvMap(rng.Light, lightEval);
        DBG_SCALE_INTENSITY_ENV_MAP(lightEval.Radiance);
    }
    else if (lightIdx < gSettings.LsdBaseEmissives)
    {
        DBG_ASSERT_EXPR(FEAT_CORE(Punctuals), DISABLED_LIGHT_INDEX);
        DBG_OUTPUT3(float3(0,1,0),  NEE_LightSampleType);

        uint punctualIdx = lightIdx - gSettings.LsdBasePunctuals;

        if (FEAT_DBG(Asserts))
        {
            uint count, _;
            gMegaBufferPunctuals.GetDimensions(count, _);
            DBG_ASSERT_LT(punctualIdx, count, OOB_LIGHT_INDEX);
        }

        PunctualLight light = gMegaBufferPunctuals[punctualIdx];

        lightSample = EvaluatePunctualLight(light, hitPos, lightEval);
        DBG_SCALE_INTENSITY_PUNCTUAL(lightEval.Radiance);
    }
    else if (lightIdx < gSettings.LsdCount)
    {
        DBG_ASSERT_EXPR(FEAT_CORE(Emission), DISABLED_LIGHT_INDEX);
        DBG_OUTPUT3(float3(0,0,1),  NEE_LightSampleType);

        uint emissiveIdx = lightIdx - gSettings.LsdBaseEmissives;

        lightSample = SampleEmissive(rng.Light, emissiveIdx, hitPos, lightEval);
        DBG_SCALE_INTENSITY_EMISSION(lightEval.Radiance);
    }
    else
    {
        DBG_ASSERT_EXPR(false, OOB_LIGHT_INDEX);
        lightEval.Radiance = 0.0f;
        return lightSample;
    }

    lightSample.Index = lightIdx;
    lightEval.PDF *= pdf_lsd;

    DBG_OUTPUT1(rng.LSD,               NEE_xi);
    DBG_OUTPUT1(pdf_lsd,               NEE_LsdPdf);
    DBG_OUTPUT1(lightSample.Index,     NEE_LightSampleIdx);

    DBG_OUTPUT1(lightEval.Distance,    NEE_LightSampleDistance);
    DBG_OUTPUT3(lightEval.Direction,   NEE_LightSampleDir);
    DBG_OUTPUT3(lightEval.Radiance,    NEE_LightSampleRadiance);
    DBG_OUTPUT1(lightEval.PDF,         NEE_LightSamplePDF);

    return lightSample;
}

LightEval EvaluateLight(
    float3 hitPos,
    float3 wo,

    LightSample lightSample)
{
    float pdf_lsd;
    EvaluateLSD(lightSample.Index, pdf_lsd);

    LightEval lightEval;

    if (lightSample.Index < gSettings.LsdBasePunctuals)
    {
        DBG_ASSERT_EXPR(FEAT_CORE(EnvironmentMap), DISABLED_LIGHT_INDEX);

        lightEval = EvaluateEnvMap(lightSample);
        DBG_SCALE_INTENSITY_ENV_MAP(lightEval.Radiance);
    }
    else if (lightSample.Index < gSettings.LsdBaseEmissives)
    {
        DBG_ASSERT_EXPR(FEAT_CORE(Punctuals), DISABLED_LIGHT_INDEX);

        uint punctualIdx = lightSample.Index - gSettings.LsdBasePunctuals;
        PunctualLight light = gMegaBufferPunctuals[punctualIdx];

        EvaluatePunctualLight(light, hitPos, lightEval);
        DBG_SCALE_INTENSITY_PUNCTUAL(lightEval.Radiance);
    }
    else if (lightSample.Index < gSettings.LsdCount)
    {
        DBG_ASSERT_EXPR(FEAT_CORE(Emission), DISABLED_LIGHT_INDEX);
        DBG_ASSERT_LT(lightSample.Index, gSettings.LsdCount, OOB_LIGHT_INDEX);
        DBG_OUTPUT3(float3(0,0,1),  NEE_LightSampleType);

        uint emissiveIdx = lightSample.Index - gSettings.LsdBaseEmissives;

        lightEval = EvaluateEmissive(emissiveIdx, hitPos, lightSample);
        DBG_SCALE_INTENSITY_EMISSION(lightEval.Radiance);
    }
    else
    {
        DBG_ASSERT_EXPR(false, OOB_LIGHT_INDEX);
        lightEval.Radiance = 0.0f;
        return lightEval;
    }

    lightEval.PDF *= pdf_lsd;

    return lightEval;
}

#endif