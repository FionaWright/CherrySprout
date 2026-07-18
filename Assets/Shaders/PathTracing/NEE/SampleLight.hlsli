#ifndef H_SAMPLE_LIGHT_H
#define H_SAMPLE_LIGHT_H

#include "PathTracing/MIS.hlsli"
#include "Scene/PunctualLight.h"
#include "Utils/Math/Punctual.hlsli"
#include "PathTracing/Debug/Scales.hlsli"

void SampleLight(
    inout RngInfo rngInfo,
    HitInfo hitInfo,
    BxDF bxdf,

    float3 wo,
    float3 nextOrigin,
    out float3 lightRadiance,
    out float pdf,
    out float3 wi
)
{
    float xi = Rand01(rngInfo);
    uint lightIdx = BinarySearch(gLightCDF, xi);

    float3 Le;
    float lightDistance;
    bool isDelta;

    if (lightIdx == 0)
    {
        DBG_ASSERT_EXPR(FEATURE_ENABLED(EnvironmentMap), DISABLED_LIGHT_INDEX);

        float xi1 = Rand01(rngInfo);
        float xi2 = Rand01(rngInfo);

        float2 uv_env;
        SampleEnvMapCdf(xi1, xi2, uv_env, wi, pdf);
        pdf *= gLightCDF[lightIdx].PMF;

        Le = gTexEnvMap.Sample(gSampler, uv_env).rgb;
        DBG_SCALE_INTENSITY_ENV_MAP(Le);
        lightDistance = INF;
        isDelta = false;
    }
    else
    {
        if (DEBUG_ENABLED(Asserts))
        {
            uint puncLightCount, _;
            gMegaBufferPunctuals.GetDimensions(puncLightCount, _);
            DBG_ASSERT_LT(lightIdx, puncLightCount+1, OOB_LIGHT_INDEX);
        }

        pdf = gLightCDF[lightIdx].PMF;

        uint punctualLightIdx = lightIdx - 1;
        PunctualLight light = gMegaBufferPunctuals[punctualLightIdx];

        LightSample lightSample = EvaluateLight(light, nextOrigin); // TODO: Is it okay to use nextOrigin instead of hitPos?
        wi = lightSample.Direction;
        Le = lightSample.Radiance;
        DBG_SCALE_INTENSITY_PUNCTUAL(Le);
        lightDistance = lightSample.Distance;
        isDelta = true;
    }

    float NdL = dot(hitInfo.Ns_ff, wi);
    if (NdL < 0.0f)
    {
        lightRadiance = 0;
        return;
    }

    bool occluded;
    TraceShadowRay(nextOrigin, wi, occluded, lightDistance);

    DBG_OUTPUT3(wi,           NEE_L_w);
    DBG_OUTPUT1(pdf,          NEE_PDF);
    DBG_OUTPUT1(occluded,     NEE_Occluded);

    if (!occluded)
    {
        float3 m;
        if (isDelta)
        {
            m = 1.0f;
        }
        else
        {
            float3 f_bxdf;
            float pdf_bxdf;
            bxdf.Evaluate(hitInfo, wo, wi, f_bxdf, pdf_bxdf);

            m = f_bxdf * PowerHeuristic(pdf, pdf_bxdf);
        }

        lightRadiance = Le * m * max(0, NdL) / pdf;

        DBG_OUTPUT3(Le,               NEE_Le);
        DBG_OUTPUT1(m,                NEE_MIS_Weight);
        return;
    }

    lightRadiance = 0;
    DBG_OUTPUT1(0,            NEE_Le);
    DBG_OUTPUT1(0,            NEE_MIS_Weight);
}

#endif