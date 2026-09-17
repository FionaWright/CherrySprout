#ifndef H_BRDF_LAMBERTIAN_LOBE_H
#define H_BRDF_LAMBERTIAN_LOBE_H

#include "PathTracing/Structs.h"

void LambertianLobe_Sample(
        HitInfo hitInfo,
        float u1, float u2,

        out float3 L_s,
        out float3 f,
        out float pdf
    )
{
    L_s = RandHemisphereCosineSSpace(u1, u2);

    float NdL = SSpaceCosTheta(L_s);

    f = hitInfo.Mat.Albedo.rgb / PI;
    pdf = NdL / PI;

    DBG_OUTPUT1(0,                BxDF_F);
    DBG_OUTPUT1(0,                BxDF_G);
}

void LambertianLobe_Evaluate(
        HitInfo hitInfo,
        float3 L_s,

        out float3 f,
        out float pdf
    )
{
    float NdL = SSpaceCosTheta(L_s);
    f = hitInfo.Mat.Albedo.rgb / PI;
    pdf = NdL / PI;
}

void LambertianLobe_Pdf(
        float3 L_s,

        out float pdf
    )
{
    float NdL = SSpaceCosTheta(L_s);
    pdf = NdL / PI;
}

#endif