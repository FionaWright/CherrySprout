#ifndef H_BRDF_LAMBERTIAN_H
#define H_BRDF_LAMBERTIAN_H

#include "BxDFs/Lobes/LambertianLobe.hlsli"

void BxDF::Sample(
    inout RngInfo rngInfo,
    HitInfo hitInfo,
    float3 wo,

    out float3 wi,
    out float3 f,
    out float pdf
)
{
    float3 L_s;

    LambertianLobe_Sample(rngInfo, hitInfo, L_s, f, pdf);

    wi = hitInfo.SFrame.ToWorld(L_s);
}

void BxDF::Evaluate(
    inout RngInfo rngInfo,
    HitInfo hitInfo,
    float3 wo,
    float3 wi,

    out float3 f,
    out float pdf
)
{
    LambertianLobe_Evaluate(rngInfo, hitInfo, wi, f, pdf);
}

#endif