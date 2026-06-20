#ifndef H_HIT_H
#define H_HIT_H

#include "PathTracing/4_GetHitInfo.hlsli"
#include "PathTracing/Debug/OutputColor.h"

#include "BxDFs/GetBxDF.hlsli"

#include "Utils/RandomDirection.h"

#include "PathTracing/MIS/SampleEnvMapCdf.hlsli"
#include "PathTracing/MIS/TraceShadowRay.hlsli"
#include "PathTracing/MIS/PowerHeuristic.hlsli"

void Hit(inout RayQuery<RAY_FLAGS> q, inout RayDesc ray, inout PathState pathState, inout float3 L_sample, inout float3 beta, inout RngInfo rngInfo)
{
    HitInfo hitInfo;
    GetHitInfo(q, hitInfo);

    float3 wo = -ray.Direction;

    float3 hitPos = ray.Origin + ray.Direction * hitInfo.RayT;
    float3 nextOrigin = hitPos + hitInfo.Ng_ff * EPSILON;

    L_sample = beta * hitInfo.Li;

    // TODO in a loose order:
    // Implement Evaluate for PBR
    // Separate f from pdf in the BxDF. Divide beta by the throughput explicitly
    // Refactor NEE sampling into functions/files
    // Perform average luminance tests between with/without NEE. Should be equal. Set up python executor

    BxDF bxdf;

    if (FEATURE_ENABLED(NEE) && FEATURE_ENABLED(EnvironmentMap))
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
            float3 L_direct = gTexEnvMap.Sample(gSampler, uv_env).rgb;

            float3 f_bxdf;
            float pdf_bxdf;
            bxdf.Evaluate(rngInfo, hitInfo, wo, wi_env, f_bxdf, pdf_bxdf);

            float m = PowerHeuristic(pdf_env, pdf_bxdf);

            L_sample += beta * L_direct * m * max(0, NdL) * f_bxdf / pdf_env;

            DBG_OUTPUT3(L_direct,     NEE_L_Direct);
            DBG_OUTPUT1(m,            NEE_MIS_Weight);
        }
        else
        {
            DBG_OUTPUT1(0,            NEE_L_Direct);
            DBG_OUTPUT1(0,            NEE_MIS_Weight);
        }
    }

    float3 wi;
    float3 f;
    float pdf;

    bxdf.Sample(rngInfo, hitInfo, wo, wi, f, pdf);

    float NdL = dot(hitInfo.Ns_ff, wi);

    beta *= f * max(0, NdL) / max(1e-6, pdf);

    pathState.LastBxdfPdf = pdf;

    DBG_OUTPUT3(f,                                 f);
    DBG_OUTPUT1(pdf,                               PDF);
    DBG_OUTPUT3(hitInfo.SFrame.ToLocal(wi),        L_s);
    DBG_OUTPUT3(wi,                                L_w);

    ray.Direction = wi;
    ray.Origin = nextOrigin;
}

#endif