#ifndef H_SAMPLE_INDIRECT_H
#define H_SAMPLE_INDIRECT_H

float3 SampleIndirectLighting(
    inout RngInfo rngInfo,
    HitInfo hitInfo,
    BxDF bxdf,
    float3 wo,

    out float3 wi,
    out float pdf,
    out bool isDirac,
    out bool isSpecular
)
{
    BxdfSample bxdfSample;
    BxdfEval bxdfEval;

    if (FEAT_DBG(BxdfTestHemisphere) && gRunBxdfTestForPixel)
    {
        float u1 = Rand01(rngInfo);
        float u2 = Rand01(rngInfo);
        wi = RandHemisphereCosineWorld(u1, u2, hitInfo.SFrame);

        bxdfEval = bxdf.Evaluate(hitInfo, wo, wi);

        float NdL = dot(hitInfo.Ns_ff, wi);
        bxdfEval.PDF = NdL / PI;
    }
    else if (FEAT_DBG(BxdfTestRevaluate) && gRunBxdfTestForPixel)
    {
        BxdfEval bxdfEvalFromSample;
        bxdfSample = bxdf.Sample(rngInfo, hitInfo, wo, bxdfEvalFromSample);

        if (bxdfSample.IsDirac)
            bxdfEval = bxdfEvalFromSample;
        else
        {
            DBG_OUTPUT_RESET();
            bxdfEval = bxdf.Evaluate(hitInfo, wo, bxdfSample.Wi);
        }

        DBG_ASSERT_APPROX(bxdfEvalFromSample.Radiance/bxdfEvalFromSample.PDF, bxdfEval.Radiance/bxdfEval.PDF, 5, REVALUATE_TEST);
    }
    else
    {
        bxdfSample = bxdf.Sample(rngInfo, hitInfo, wo, bxdfEval);
    }

    wi = bxdfSample.Wi;
    isDirac = bxdfSample.IsDirac;
    isSpecular = bxdfSample.IsSpecular;

    pdf = bxdfEval.PDF;

    float NdL = dot(hitInfo.Ns_ff, bxdfSample.Wi);

    DBG_OUTPUT3(bxdfEval.Radiance,                 BxDF_f);
    DBG_OUTPUT1(bxdfEval.PDF,                      BxDF_PDF);
    DBG_OUTPUT3(bxdfSample.Wi,                     BxDF_L_w);

    return bxdfEval.Radiance * abs(NdL) / max(1e-6, bxdfEval.PDF);
}

#endif