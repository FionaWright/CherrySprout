#ifndef H_BXDF_INTERFACE_H
#define H_BXDF_INTERFACE_H

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
        float3 wi,
        LobeTag lobe
    );

    uint LobeSelectRngCount();
    uint MicrofacetModelRngCount();
};

#endif