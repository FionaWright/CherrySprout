#ifndef H_RESOLVE_BXDF_H
#define H_RESOLVE_BXDF_H

#include "Utils/RandomDirection.h"
#include "MicrofacetModels/MicrofacetUtils.hlsli"
#include "BxDFs/BxDFMode.h"
#include "PathTracing/Structs.h"

struct RngInfo;

struct BxDF
{
    void Sample(
        inout RngInfo rngInfo,
        inout PathState pathState,
        HitInfo hitInfo,
        float3 wo,

        out float3 wi,
        out float3 f,
        out float pdf
    );

    void Evaluate(
        HitInfo hitInfo,
        float3 wo,
        float3 wi,

        out float3 f,
        out float pdf
    );
};

#if   (BXDF_ENABLED(LAMBERTIAN))
#    include "BxDFs/BRDF_Lambertian.hlsli"

#elif (BXDF_ENABLED(PBR))
#    include "BxDFs/BSDF_PBR.hlsli"

#elif (BXDF_ENABLED(PRINCIPLED))
// TODO

#endif

#endif