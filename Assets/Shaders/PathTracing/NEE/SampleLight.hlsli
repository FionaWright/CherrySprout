#ifndef H_SAMPLE_LIGHT_H
#define H_SAMPLE_LIGHT_H

#include "PathTracing/MIS.hlsli"

void SampleLight(
    inout RngInfo rngInfo,
    HitInfo hitInfo,
    BxDF bxdf,

    float3 wo,
    float3 nextOrigin,
    float3 beta,
    out float3 lightRadiance
)
{
    float u1 = Rand01(rngInfo);
    float u2 = Rand01(rngInfo);

    float2 uv_env;
    float3 wi_env;
    float pdf_env;
    SampleEnvMapCdf(u1, u2, uv_env, wi_env, pdf_env);

    float NdL = dot(hitInfo.Ns_ff, wi_env);

    bool occluded;
    TraceShadowRay(nextOrigin, wi_env, occluded);

    DBG_OUTPUT3(wi_env,       NEE_L_w);
    DBG_OUTPUT1(pdf_env,      NEE_PDF);
    DBG_OUTPUT1(occluded,     NEE_Occluded);

    if (!occluded && NdL >= 0)
    {
        float3 Le = gTexEnvMap.Sample(gSampler, uv_env).rgb;

        float3 f_bxdf;
        float pdf_bxdf;
        bxdf.Evaluate(hitInfo, wo, wi_env, f_bxdf, pdf_bxdf);

        float m = PowerHeuristic(pdf_env, pdf_bxdf);

        lightRadiance = beta * Le * m * max(0, NdL) * f_bxdf / pdf_env;

        DBG_OUTPUT3(Le,               NEE_Le);
        DBG_OUTPUT1(m,                NEE_MIS_Weight);
        return;
    }

    lightRadiance = 0;

    DBG_OUTPUT1(0,            NEE_Le);
    DBG_OUTPUT1(0,            NEE_MIS_Weight);
}

#endif