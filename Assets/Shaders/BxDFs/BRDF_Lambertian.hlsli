#ifndef H_BRDF_LAMBERTIAN_H
#define H_BRDF_LAMBERTIAN_H

#include "PathTracing/Debug/Assert.hlsli"
#include "PathTracing/Debug/OutputColorMacros.hlsli"
#include "PathTracing/Structs.h"
#include "BxDFs/Lobes/LambertianLobe.hlsli"

void BxDF::Sample(
    inout RngInfo rngInfo,
    inout PathState pathState,
    HitInfo hitInfo,
    float3 wo,

    out float3 wi,
    out float3 f,
    out float pdf
)
{
    float u1 = Rand01(rngInfo);
    float u2 = Rand01(rngInfo);

    float3 L_s;
    LambertianLobe_Sample(hitInfo, u1, u2, L_s, f, pdf);

    wi = hitInfo.SFrame.ToWorld(L_s);

    DBG_OUTPUT3(L_s,        BxDF_L_s);
}

void BxDF::Evaluate(
    HitInfo hitInfo,
    float3 wo,
    float3 wi,

    out float3 f,
    out float pdf
)
{
    float3 L_s = hitInfo.SFrame.ToLocal(wi);
    LambertianLobe_Evaluate(hitInfo, L_s, f, pdf);

    DBG_OUTPUT3(L_s,        Eval_L_s);
}

#endif