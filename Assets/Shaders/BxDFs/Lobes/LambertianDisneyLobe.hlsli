#ifndef H_BRDF_LAMBERTIAN_DISNEY_LOBE_H
#define H_BRDF_LAMBERTIAN_DISNEY_LOBE_H

#include "BxDFs/Lobes/LambertianLobe.hlsli"
#include "PathTracing/Structs.h"

void LambertianDisneyLobe_Sample(
        HitInfo hitInfo,
        float u1, float u2,

        out float3 L_s,
        out float3 f,
        out float pdf
    )
{
    LambertianLobe_Sample(hitInfo, u1, u2, L_s, f, pdf);

    f *= (1.0 - hitInfo.Mat.Metallic);
}

void LambertianDisneyLobe_Evaluate(
        HitInfo hitInfo,
        float3 L_s,

        out float3 f,
        out float pdf
    )
{
    LambertianLobe_Evaluate(hitInfo, L_s, f, pdf);
    f *= (1.0 - hitInfo.Mat.Metallic);
}

void LambertianDisneyLobe_Pdf(
        float3 L_s,

        out float pdf
    )
{
    LambertianLobe_Pdf(L_s, pdf);
}

#endif