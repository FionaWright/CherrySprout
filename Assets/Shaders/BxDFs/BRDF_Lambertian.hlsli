#ifndef H_BRDF_LAMBERTIAN_H
#define H_BRDF_LAMBERTIAN_H

void BxDF::Sample(
    inout RngInfo rngInfo,
    HitInfo hitInfo,
    float3 wo,

    out float3 wi,
    out float3 f,
    out float pdf
)
{
    float u1 = Rand01(rngInfo);
    float u2 = Rand01(rngInfo);

    wi = RandHemisphereCosineWorld(u1, u2, hitInfo.SFrame);

    float NdL = dot(hitInfo.Ns_ff, wi);
    f = hitInfo.Mat.Albedo.rgb / PI;
    pdf = NdL / PI;
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
    float NdL = dot(hitInfo.Ns_ff, wi);
    f = hitInfo.Mat.Albedo.rgb / PI;
    pdf = NdL / PI;
}

#endif