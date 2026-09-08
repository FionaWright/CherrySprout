#ifndef H_HIT_H
#define H_HIT_H

#include "PathTracing/Debug/OutputColorMacros.hlsli"

#include "PathTracing/HitInfo/GetHitInfo.hlsli"
#include "BxDFs/GetBxDF.hlsli"

#include "Utils/RandomDirection.h"

#include "PathTracing/NEE/SampleEnvMapCdf.hlsli"
#include "PathTracing/NEE/TraceShadowRay.hlsli"
#include "PathTracing/NEE/SampleLight.hlsli"

#include "PathTracing/ReSTIR/ReSTIR_DI.hlsli"
#include "PathTracing/ReSTIR/ReservoirBuffer.hlsli"

void Hit(HitInfo hitInfo, inout RayDesc ray, inout PathState pathState, inout float3 L_sample, inout float3 beta, inout RngInfo rngInfo, uint2 pixelCoord)
{
    float3 hitPos = ray.Origin + ray.Direction * hitInfo.RayT;
    float3 nextOrigin = hitPos + hitInfo.Ng_ff * EPSILON;

#if FEATURE_ENABLED(Transient)
    pathState.RollingPathDistance += hitInfo.RayT;
#endif

    L_sample = beta * hitInfo.Emission;

    float3 wo = -ray.Direction;

    // TODO: Perform average luminance tests between with/without NEE. Should be equal. Set up python executor

    BxDF bxdf;

    if (FEATURE_ENABLED(NEE))
    {
        float3 E_direct = 0;

        if (FEATURE_ENABLED(RestirDI) && pathState.RaySegmentIdx == 0)
        {
            // TODO: Share TraceShadowRay?
            E_direct += SampleReservoir(hitInfo, pixelCoord, wo, nextOrigin, bxdf);
        }
        else
        {
            float3 wi_env;
            float pdf_env;
            SampleLight(rngInfo, hitInfo, pathState, bxdf, wo, nextOrigin, E_direct, pdf_env, wi_env);
        }

        // TODO: Share shadow ray handling for both out here
        L_sample += E_direct * beta;
    }

    float3 wi;
    float3 f;
    float pdf;

    // TODO: Move these tests into own file or something
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
    DBG_OUTPUT3(wi,                                L_w);

    ray.Direction = wi;
    ray.Origin = nextOrigin;
}

#endif