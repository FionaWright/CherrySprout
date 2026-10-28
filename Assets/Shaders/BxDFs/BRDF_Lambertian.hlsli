#ifndef H_BRDF_LAMBERTIAN_H
#define H_BRDF_LAMBERTIAN_H

#include "PathTracing/Debug/Assert.hlsli"
#include "PathTracing/Debug/Internal/OutputColorMacros.hlsli"
#include "PathTracing/Structs.h"
#include "BxDFs/Lobes/LambertianLobe.hlsli"

BxdfSample BxDF::Sample(
    RngValuesAncillary::RngValuesBxDF rng,
    HitInfo hitInfo,
    float3 wo,

    out BxdfEval eval)
{
    float3 L_s;
    LambertianLobe_Sample(hitInfo, rng.Microfacet.x, rng.Microfacet.y, L_s, eval.Radiance, eval.PDF);

    BxdfSample sample;
    sample.Wi = hitInfo.SFrame.ToWorld(L_s);
    sample.IsDirac = false;
    sample.Lobe = LobeTag::eLambertian;

    DBG_OUTPUT3(L_s,        BxDF_L_s);

    return sample;
}

BxdfEval BxDF::Evaluate(
    HitInfo hitInfo,
    float3 wo,
    float3 wi,
    LobeTag lobe)
{
    BxdfEval eval;

    if (!CoversLobe(lobe, LobeTag::eLambertian))
    {
        eval.Radiance = 0.0f;
        eval.PDF = 0.0f;
        return eval;
    }

    float3 L_s = hitInfo.SFrame.ToLocal(wi);
    DBG_OUTPUT3(L_s,        Eval_L_s);

    LambertianLobe_Evaluate(hitInfo, L_s, eval.Radiance, eval.PDF);

    return eval;
}

uint BxDF::MicrofacetModelRngCount()
{
    return 2;
}

uint BxDF::LobeSelectRngCount()
{
    return 0;
}

#endif