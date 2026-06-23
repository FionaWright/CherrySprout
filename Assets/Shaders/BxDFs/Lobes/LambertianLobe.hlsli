#ifndef H_BRDF_LAMBERTIAN_LOBE_H
#define H_BRDF_LAMBERTIAN_LOBE_H

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

    DBG_OUTPUT1(0,                F);
    DBG_OUTPUT1(0,                G);
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

#endif