#ifndef H_SAMPLE_LIGHT_H
#define H_SAMPLE_LIGHT_H

#include "PathTracing/MIS.hlsli"
#include "PathTracing/NEE/BinarySearch.hlsli"
#include "PathTracing/NEE/SampleEnvMapCdf.hlsli"
#include "PathTracing/NEE/TraceShadowRay.hlsli"
#include "Scene/PunctualLight.h"
#include "Utils/Math/Punctual.hlsli"
#include "PathTracing/Debug/Scales.hlsli"
#include "PathTracing/Transient.hlsli"

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

void SampleLight(
    inout RngInfo rngInfo,
    HitInfo hitInfo,
    PathState pathState,
    BxDF bxdf,

    float3 wo,
    float3 nextOrigin,
    out float3 lightRadiance,
    out float pdf,
    out float3 wi
)
{
    float xi = Rand01(rngInfo);
    uint lightIdx;
    float pdf_lsd;
    sampleLSD(xi, lightIdx, pdf_lsd);

    float3 Le;
    float lightDistance;
    bool isDelta;

    if (lightIdx == 0)
    {
        DBG_ASSERT_EXPR(FEATURE_ENABLED(EnvironmentMap), DISABLED_LIGHT_INDEX);

        float xi1 = Rand01(rngInfo);
        float xi2 = Rand01(rngInfo);

        float2 uv_env;
        float pdf_env;
        SampleEnvMapCdf(xi1, xi2, uv_env, wi, pdf_env);
        pdf = pdf_env * pdf_lsd;

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

        pdf = pdf_lsd;

        uint punctualLightIdx = lightIdx - 1;
        PunctualLight light = gMegaBufferPunctuals[punctualLightIdx];

        LightSample lightSample = EvaluateLight(light, nextOrigin); // TODO: Is it okay to use nextOrigin instead of hitPos?
        wi = lightSample.Direction;
        Le = lightSample.Radiance;
        DBG_SCALE_INTENSITY_PUNCTUAL(Le);
        lightDistance = lightSample.Distance;
        isDelta = true;
    }

    DBG_OUTPUT3(Le,               NEE_Le);
    DBG_OUTPUT1(lightIdx,         NEE_LightIdx);
    DBG_OUTPUT3(xi,               NEE_xi);
    DBG_OUTPUT1(lightDistance,    NEE_Distance);
    DBG_OUTPUT3(wi,               NEE_L_w);
    DBG_OUTPUT1(pdf,              NEE_PDF);

    if (FEATURE_ENABLED(Transient))
    {
        if (gSettings.TransientLightIdx == -1 || gSettings.TransientLightIdx == lightIdx)
        {
            float transientFactor = GetTransientFactor(pathState.RollingPathDistance + lightDistance);
            Le *= transientFactor;
        }
        else
            Le = 0.0f;
    }

    float NdL = dot(hitInfo.Ns_ff, wi);
    if (length(Le) == 0.0f || NdL < 0.0f)
    {
        lightRadiance = 0;
        return;
    }

    bool occluded;
    TraceShadowRay(nextOrigin, wi, occluded, lightDistance);

    DBG_OUTPUT1(occluded,     NEE_Occluded);

    if (occluded)
    {
        lightRadiance = 0;
        DBG_OUTPUT1(0,            NEE_MIS_Weight);
        DBG_OUTPUT1(0,            NEE_Radiance);
        return;
    }

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

    DBG_OUTPUT1(m,                NEE_MIS_Weight);
    DBG_OUTPUT1(lightRadiance,    NEE_Radiance);
}

#endif