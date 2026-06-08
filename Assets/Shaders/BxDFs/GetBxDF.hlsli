#ifndef H_RESOLVE_BXDF_H
#define H_RESOLVE_BXDF_H

#include "Utils/RandomDirection.h"
#include "MicrofacetModels/MicrofacetUtils.hlsli"

struct BxDF
{
    void Sample(
        inout RngInfo rngInfo,
        HitInfo hitInfo,
        float3 wo,

        out float3 wi,
        out float3 f,
        out float pdf
    );

    void Evaluate(
        inout RngInfo rngInfo,
        HitInfo hitInfo,
        float3 wi,

        out float3 f,
        out float pdf
    );
};

#include "BxDFs/Lambertian.hlsli"

#endif