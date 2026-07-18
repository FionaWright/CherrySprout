#ifndef H_HIT_H
#define H_HIT_H

#include "PathTracing/Debug/OutputColorMacros.hlsli"

#include "PathTracing/4_GetHitInfo.hlsli"
#include "BxDFs/GetBxDF.hlsli"

#include "Utils/RandomDirection.h"

#   include "PathTracing/NEE/SampleEnvMapCdf.hlsli"
#   include "PathTracing/NEE/TraceShadowRay.hlsli"
#   include "PathTracing/NEE/SampleLight.hlsli"

#   include "PathTracing/ReSTIR/ReSTIR_DI.hlsli"
#   include "PathTracing/ReSTIR/ReservoirBuffer.hlsli"

void Hit(inout RayQuery<RAY_FLAGS> q, inout RayDesc ray, inout PathState pathState, inout float3 L_sample, inout float3 beta, inout RngInfo rngInfo, uint2 pixelCoord)
{
    HitInfo hitInfo;
    GetHitInfo(q, hitInfo);

    float3 hitPos = ray.Origin + ray.Direction * hitInfo.RayT;
    float3 nextOrigin = hitPos + hitInfo.Ng_ff * EPSILON;

    L_sample = beta * hitInfo.Li;

    float3 wo = -ray.Direction;

    // TODO in a loose order:
    // Perform average luminance tests between with/without NEE. Should be equal. Set up python executor
    // Make mocks for NEE and restir so the include files can be ignored when disabled

    BxDF bxdf;

    if (FEATURE_ENABLED(NEE))
    {
        float3 E_direct = 0;
        if (FEATURE_ENABLED(RestirDI) && pathState.RaySegmentIdx == 0)
        {
#if FEATURE_ENABLED(RestirDI)
            uint reservoirIdx = GetReservoirBufferIndex_Current(pixelCoord);
            ReservoirDI reservoir = gReservoirBuffer[reservoirIdx];

            float3 wi = reservoir.Y.Direction;
            float NdL = max(0, dot(wi, hitInfo.Ns_ff));
            if (reservoir.Confidence > 0 && NdL > 0)
            {
                bool occluded;
                TraceShadowRay(nextOrigin, wi, occluded);

                if (!occluded)
                {
                    float3 f_bxdf;
                    float pdf_bxdf;
                    bxdf.Evaluate(hitInfo, wo, wi, f_bxdf, pdf_bxdf);

                    float W_Y = reservoir.W_Y / reservoir.Confidence; // ?
                    E_direct = f_bxdf * reservoir.Y.Radiance * NdL * W_Y;
                }
            }
#endif
        }
        else
        {
            float3 wi_env;
            float pdf_env;
            SampleLight(rngInfo, hitInfo, bxdf, wo, nextOrigin, E_direct, pdf_env, wi_env);
        }

        // TODO: Share shadow ray handling for both out here
        L_sample += E_direct * beta;
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
    DBG_OUTPUT3(wi,                                L_w);

    ray.Direction = wi;
    ray.Origin = nextOrigin;
}

#endif