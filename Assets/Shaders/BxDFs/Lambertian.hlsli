#ifndef H_LAMBERTIAN_H
#define H_LAMBERTIAN_H

struct BxDF
{
#include "BxDFs/IBxDF.hlsli"
};

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

    ShadingFrame sframe = CreateShadingFrame(hitInfo.Ns_ff);
    wi = RandHemisphereCosineWorld(u1, u2, sframe);

    float NdL = dot(hitInfo.Ns_ff, wi);
    f = hitInfo.Albedo;
    pdf = NdL / PI;
}

void BxDF::Evaluate(
    inout RngInfo rngInfo,
    HitInfo hitInfo,
    float3 wi,

    out float3 f,
    out float pdf
)
{
    float NdL = dot(hitInfo.Ns_ff, wi);
    f = hitInfo.Albedo;
    pdf = NdL / PI;
}

#endif