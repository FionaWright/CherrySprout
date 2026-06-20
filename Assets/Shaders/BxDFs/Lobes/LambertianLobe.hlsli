#ifndef H_BRDF_LAMBERTIAN_LOBE_H
#define H_BRDF_LAMBERTIAN_LOBE_H

void LambertianLobe_Sample(
        inout RngInfo rngInfo,
        HitInfo hitInfo,

        out float3 L_s,
        out float3 f,
        out float pdf
    )
{
    float u1 = Rand01(rngInfo);
    float u2 = Rand01(rngInfo);

    L_s = RandHemisphereCosineSSpace(u1, u2);

    float NdL = SSpaceCosTheta(L_s);

    f = hitInfo.Mat.Albedo.rgb / PI;
    pdf = NdL / PI;

    DBG_OUTPUT1(0,                F);
    DBG_OUTPUT1(0,                G);
}

void LambertianLobe_Evaluate(
        HitInfo hitInfo,
        float3 wi,

        out float3 f,
        out float pdf
    )
{
    float NdL = dot(hitInfo.Ns_ff, wi);
    f = hitInfo.Mat.Albedo.rgb / PI;
    pdf = NdL / PI;
}

#endif