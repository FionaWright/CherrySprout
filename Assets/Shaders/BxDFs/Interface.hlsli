#ifndef H_BXDF_INTERFACE_H
#define H_BXDF_INTERFACE_H

#include "PathTracing/RandomSampleStructs.h"

struct BxDF
{
    BxdfSample Sample(
        RngValuesAncillary::RngValuesBxDF rng,
        HitInfo hitInfo,
        float3 wo,

        out BxdfEval eval
    );

    BxdfEval Evaluate(
        HitInfo hitInfo,
        float3 wo,
        float3 wi,
        LobeTag lobe
    );

    uint LobeSelectRngCount();
    uint MicrofacetModelRngCount();
};

#endif