#ifndef H_RESOLVE_BXDF_H
#define H_RESOLVE_BXDF_H

#include "Utils/RandomDirection.h"
#include "MicrofacetModels/MicrofacetUtils.hlsli"
#include "BxDFs/BxDFMode.h"
#include "BxDFs/Structs.h"
#include "PathTracing/Structs.h"

struct RngInfo;

struct BxDF
{
    BxdfSample Sample(
        inout RngInfo rngInfo,
        HitInfo hitInfo,
        float3 wo,

        out BxdfEval eval
    );

    BxdfEval Evaluate(
        HitInfo hitInfo,
        float3 wo,
        float3 wi
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