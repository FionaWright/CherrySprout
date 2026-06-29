#ifndef H_HIT_H
#define H_HIT_H

#include "PathTracing/4_GetHitInfo.hlsli"
#include "PathTracing/Debug/OutputColor.h"

#include "BxDFs/GetBxDF.hlsli"

#include "Utils/RandomDirection.h"

#include "PathTracing/NEE/SampleEnvMapCdf.hlsli"
#include "PathTracing/NEE/TraceShadowRay.hlsli"
#include "PathTracing/NEE/SampleLight.hlsli"

void Hit(inout RayQuery<RAY_FLAGS> q, inout RayDesc ray, inout PathState pathState, inout float3 L_sample, inout float3 beta, inout RngInfo rngInfo)
{
    HitInfo hitInfo;
    GetHitInfo(q, hitInfo);

    float3 wo = -ray.Direction;

    float3 hitPos = ray.Origin + ray.Direction * hitInfo.RayT;
    float3 nextOrigin = hitPos + hitInfo.Ng_ff * EPSILON;

    L_sample = beta * hitInfo.Li;

    // TODO in a loose order:
    // Perform average luminance tests between with/without NEE. Should be equal. Set up python executor

    BxDF bxdf;

    if (FEATURE_ENABLED(NEE) && FEATURE_ENABLED(EnvironmentMap))
    {
        float3 lightRadiance;
        SampleLight(rngInfo, hitInfo, bxdf, wo, nextOrigin, beta, lightRadiance);
        L_sample += lightRadiance;
    }

    float3 wi;
    float3 f;
    float pdf;

    if (DEBUG_ENABLED(BxdfTestHemisphere) && gRunBxdfTestForPixel)
    {
        float u1 = Rand01(rngInfo);
        float u2 = Rand01(rngInfo);
        wi = RandHemisphereCosineWorld(u1, u2, hitInfo.SFrame);

        float bxdfPdf; // Discarded
        bxdf.Evaluate(hitInfo, wo, wi, f, bxdfPdf);

        float NdL = dot(hitInfo.Ns_ff, wi);
        pdf = NdL / PI;
    }
    else if (DEBUG_ENABLED(BxdfTestRevaluate) && gRunBxdfTestForPixel)
    {
        float3 f_sample;
        float pdf_sample;
        bxdf.Sample(rngInfo, pathState, hitInfo, wo, wi, f_sample, pdf_sample);

        if (pathState.LastRayDiracDelta)
        {
            f = f_sample;
            pdf = pdf_sample;
        }
        else
        {
            DBG_OUTPUT_RESET();
            bxdf.Evaluate(hitInfo, wo, wi, f, pdf);
        }

        DBG_ASSERT_APPROX(f_sample, f, 5, REVALUATE_F);
        DBG_ASSERT_APPROX(pdf_sample, pdf, 5, REVALUATE_PDF);
    }
    else
    {
        bxdf.Sample(rngInfo, pathState, hitInfo, wo, wi, f, pdf);
    }

    float NdL = dot(hitInfo.Ns_ff, wi);
    beta *= f * abs(NdL) / max(1e-6, pdf);

    pathState.LastBxdfPdf = pdf;

    DBG_OUTPUT3(f,                                 f);
    DBG_OUTPUT1(pdf,                               PDF);
    DBG_OUTPUT3(hitInfo.SFrame.ToLocal(wi),        L_s);
    DBG_OUTPUT3(wi,                                L_w);

    ray.Direction = wi;
    ray.Origin = nextOrigin;
}

#endif