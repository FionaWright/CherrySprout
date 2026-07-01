#ifndef H_SAMPLE_LIGHT_H
#define H_SAMPLE_LIGHT_H

#include "PathTracing/MIS.hlsli"

void SampleLight(
    inout RngInfo rngInfo,
    HitInfo hitInfo,
    BxDF bxdf,

    float3 wo,
    float3 nextOrigin,
    out float3 lightRadiance,
    out float pdf,
    out float3 wi,
    out float distance
)
{
    float u1 = Rand01(rngInfo);
    float u2 = Rand01(rngInfo);

    float2 uv_env;
    SampleEnvMapCdf(u1, u2, uv_env, wi, pdf);

    float NdL = dot(hitInfo.Ns_ff, wi);

    bool occluded;
    TraceShadowRay(nextOrigin, wi, occluded);

    DBG_OUTPUT3(wi,           NEE_L_w);
    DBG_OUTPUT1(pdf,      NEE_PDF);
    DBG_OUTPUT1(occluded,     NEE_Occluded);

    if (!occluded && NdL >= 0) // TODO: Move NDL earlier
    {
        float3 Le = gTexEnvMap.Sample(gSampler, uv_env).rgb;

        float3 f_bxdf;
        float pdf_bxdf;
        bxdf.Evaluate(hitInfo, wo, wi, f_bxdf, pdf_bxdf);

        float m = PowerHeuristic(pdf, pdf_bxdf);

        lightRadiance = Le * m * max(0, NdL) * f_bxdf / pdf;

        DBG_OUTPUT3(Le,               NEE_Le);
        DBG_OUTPUT1(m,                NEE_MIS_Weight);
        return;
    }

    lightRadiance = 0;

    DBG_OUTPUT1(0,            NEE_Le);
    DBG_OUTPUT1(0,            NEE_MIS_Weight);
}

#endif