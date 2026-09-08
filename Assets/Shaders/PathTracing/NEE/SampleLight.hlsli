#ifndef H_SAMPLE_LIGHT_H
#define H_SAMPLE_LIGHT_H

#include "PathTracing/MIS.hlsli"
#include "PathTracing/NEE/BinarySearch.hlsli"
#include "PathTracing/NEE/SampleEnvMapCdf.hlsli"
#include "PathTracing/NEE/TraceShadowRay.hlsli"
#include "Scene/PunctualLight.h"
#include "Utils/Math/Punctual.hlsli"
#include "PathTracing/Debug/Scales.hlsli"

#if FEATURE_ENABLED(AliasTables)
void sampleLSD(float xi, out uint lightIdx, out float pdf)
{
    uint lightCount, _;
    gLightAliasTable.GetDimensions(lightCount, _);

    xi *= lightCount;
    uint xiIdx = floor(xi);
    float xi01 = frac(xi);

    AliasEntry entry = gLightAliasTable[xiIdx];
    lightIdx = xi01 < entry.Threshold ? xiIdx : entry.Alias;

    pdf = gLightAliasTable[lightIdx].PDF;
}
#else
void sampleLSD(float xi, out uint lightIdx, out float pdf)
{
    lightIdx = BinarySearch(gLightCDF, xi);
    pdf = gLightCDF[lightIdx].PDF;
}
#endif

LightSample SampleLight(
    inout RngInfo rngInfo,
    HitInfo hitInfo,
    PathState pathState,
    BxDF bxdf,

    float3 wo,
    float3 hitPos,
    float3 nextOrigin
)
{
    float xi = Rand01(rngInfo);
    float pdf_lsd;
    uint lightIdx;
    sampleLSD(xi, lightIdx, pdf_lsd);

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

        lightSample = EvaluateLight(light, hitPos);
        DBG_SCALE_INTENSITY_PUNCTUAL(lightSample.Radiance);

        lightSample.PDF = pdf_lsd;
        lightSample.IsDelta = true;
    }

    lightSample.Index = lightIdx;

    DBG_OUTPUT3(lightSample.Radiance, NEE_Le);
    DBG_OUTPUT1(lightSample.Index, NEE_LightIdx);
    DBG_OUTPUT3(xi, NEE_xi);
    DBG_OUTPUT1(lightSample.Distance, NEE_Distance);
    DBG_OUTPUT3(lightSample.Direction, NEE_L_w);
    DBG_OUTPUT1(lightSample.PDF, NEE_PDF);

    return lightSample;
}

#endif