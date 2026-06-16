#ifndef H_HIT_H
#define H_HIT_H

#include "PathTracing/4_GetHitInfo.hlsli"
#include "PathTracing/Debug/OutputColor.h"

#include "BxDFs/GetBxDF.hlsli"

#include "Utils/RandomDirection.h"

#include "PathTracing/MIS/SampleEnvMapCdf.hlsli"
#include "PathTracing/MIS/TraceShadowRay.hlsli"
#include "PathTracing/MIS/PowerHeuristic.hlsli"

void Hit(inout RayQuery<RAY_FLAGS> q, inout RayDesc ray, inout float3 L_sample, inout float3 beta, inout RngInfo rngInfo)
{
    HitInfo hitInfo;
    GetHitInfo(q, hitInfo);

    float3 wo = -ray.Direction;

    float3 hitPos = ray.Origin + ray.Direction * hitInfo.RayT;
    float3 nextOrigin = hitPos + hitInfo.Ng_ff * EPSILON;

    L_sample = beta * hitInfo.Li;

    // TODO in a loose order:
    // Switch to Lambert until proven working for that
    // Test direct lighting only with no indirect and no MIS (weight=0?)
    // Add MIS weight handling in the Miss() function. Might require carrying information from the previous ray
    // Implement Evaluate for PBR
    // Double check beta *= f is correct. Should it not be divided by the pdf?

    BxDF bxdf;

    if (FEATURE_ENABLED(DirectLighting))
    {
        float u1 = Rand01(rngInfo);
        float u2 = Rand01(rngInfo);

        float3 wi_env;
        float pdf_env;
        SampleEnvMapCdf(u1, u2, wi_env, pdf_env);

        bool occluded;
        TraceShadowRay(nextOrigin, wi_env, occluded);

        if (!occluded)
        {
            float2 uv = EaSphereToSquare(wi_env);
            float3 L_direct = gTexEnvMap.Sample(gSampler, uv).rgb;

            float3 f_bxdf;
            float pdf_bxdf;
            bxdf.Evaluate(rngInfo, hitInfo, wo, wi_env, f_bxdf, pdf_bxdf);

            float weight = PowerHeuristic(pdf_env, pdf_bxdf);

            float NdL = dot(hitInfo.Ns_ff, wi_env); // TODO: Need to max/abs?
            L_sample += beta * L_direct * weight * NdL * f_bxdf / pdf_env;
        }
    }

    float3 wi;
    float3 f;
    float pdf;

    bxdf.Sample(rngInfo, hitInfo, wo, wi, f, pdf);
    beta *= f;

    DBG_OUTPUT3(f,                                 f);
    DBG_OUTPUT1(pdf,                               PDF);
    DBG_OUTPUT3(hitInfo.SFrame.ToLocal(wi),        L_s);
    DBG_OUTPUT3(wi,                                L_w);

    ray.Direction = wi;
    ray.Origin = nextOrigin;
}

#endif