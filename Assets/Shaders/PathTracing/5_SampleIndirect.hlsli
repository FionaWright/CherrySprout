#ifndef H_SAMPLE_INDIRECT_H
#define H_SAMPLE_INDIRECT_H

#include "PathTracing/RandomSample.hlsli"

float3 SampleIndirectLighting(
    RngValuesAncillary::RngValuesBxDF rng,
    HitInfo hitInfo,
    float3 wo,

    out float3 wi,
    out float pdf,
    out bool isDirac,
    out LobeTag lobe
)
{
    BxdfSample bxdfSample;
    BxdfEval bxdfEval;

    if (FEAT_DBG(BxdfTestHemisphere) && gRunBxdfTestForPixel)
    {
        wi = RandHemisphereCosineWorld(rng.Microfacet.x, rng.Microfacet.y, hitInfo.SFrame);

        bxdfEval = BxDF::Evaluate(hitInfo, wo, wi, LobeTag::eAll);

        float NdL = dot(hitInfo.Ns_ff, wi);
        bxdfEval.PDF = NdL / PI;
    }
    else if (FEAT_DBG(BxdfTestRevaluate) && gRunBxdfTestForPixel)
    {
        BxdfEval bxdfEvalFromSample;
        bxdfSample = BxDF::Sample(rng, hitInfo, wo, bxdfEvalFromSample);

        if (bxdfSample.IsDirac)
            bxdfEval = bxdfEvalFromSample;
        else
        {
            DBG_OUTPUT_RESET();
            bxdfEval = BxDF::Evaluate(hitInfo, wo, bxdfSample.Wi, bxdfSample.Lobe);
        }

        DBG_ASSERT_APPROX(bxdfEvalFromSample.Radiance/bxdfEvalFromSample.PDF, bxdfEval.Radiance/bxdfEval.PDF, 5, REVALUATE_TEST);
    }
    else if (FEAT_DBG(BxdfTestLobe))
    {
#define ROUGH_1 0.3f
//#define ROUGH_1 max(0.1f, hitInfo.Mat.Roughness)
#define ROUGH_2 0.1f
        if (gRunBxdfTestForPixel)
            hitInfo.Mat.Roughness = ROUGH_1;
        else
            hitInfo.Mat.Roughness = ROUGH_2;

        bxdfSample = BxDF::Sample(rng, hitInfo, wo, bxdfEval);
        bxdfEval = BxDF::Evaluate(hitInfo, wo, bxdfSample.Wi, bxdfSample.Lobe);

        if (bxdfEval.PDF > 0.0f && !bxdfSample.IsDirac)
        {
            float samplingPDF = bxdfEval.PDF;

            hitInfo.Mat.Roughness = ROUGH_2;
            bxdfEval = BxDF::Evaluate(hitInfo, wo, bxdfSample.Wi, bxdfSample.Lobe);

            bxdfEval.PDF = samplingPDF;
        }
    }
    else
    {
        bxdfSample = BxDF::Sample(rng, hitInfo, wo, bxdfEval);
    }

    wi = bxdfSample.Wi;
    isDirac = bxdfSample.IsDirac;
    lobe = bxdfSample.Lobe;

    pdf = bxdfEval.PDF;

    float NdL = dot(hitInfo.Ns_ff, bxdfSample.Wi);

    DBG_OUTPUT3(bxdfEval.Radiance,                 BxDF_f);
    DBG_OUTPUT1(bxdfEval.PDF,                      BxDF_PDF);
    DBG_OUTPUT3(bxdfSample.Wi,                     BxDF_L_w);

    if (bxdfEval.PDF == 0.0f)
        return 0.0f;

    return bxdfEval.Radiance * abs(NdL) / max(1e-6, bxdfEval.PDF);
}

#endif