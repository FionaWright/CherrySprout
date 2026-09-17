#ifndef H_SAMPLE_INDIRECT_H
#define H_SAMPLE_INDIRECT_H

float3 SampleIndirectLighting(
    inout RngInfo rngInfo,
    HitInfo hitInfo,
    BxDF bxdf,
    PathState pathState,
    float3 wo,

    out float3 wi,
    out float pdf
)
{
    float3 f;

    if (DEBUG_ENABLED(BxdfTestHemisphere) && gRunBxdfTestForPixel)
    {
        float u1 = Rand01(rngInfo);
        float u2 = Rand01(rngInfo);
        wi = RandHemisphereCosineWorld(u1, u2, hitInfo.SFrame);

        float bxdfPdf; // Discarded
        bxdf.Evaluate(hitInfo, wo, wi, f, bxdfPdf);

        float NdL = dot(hitInfo.Ns_ff, wi);
        pdf = NdL / PI;
    }
    else if (DEBUG_ENABLED(BxdfTestRevaluate) && gRunBxdfTestForPixel)
    {
        float3 f_sample;
        float pdf_sample;
        bxdf.Sample(rngInfo, pathState, hitInfo, wo, wi, f_sample, pdf_sample);

        if (pathState.LastRayWasDiracDelta)
        {
            f = f_sample;
            pdf = pdf_sample;
        }
        else
        {
            DBG_OUTPUT_RESET();
            bxdf.Evaluate(hitInfo, wo, wi, f, pdf);
        }

        DBG_ASSERT_APPROX(f_sample, f, 5, REVALUATE_F);
        DBG_ASSERT_APPROX(pdf_sample, pdf, 5, REVALUATE_PDF);
    }
    else
    {
        bxdf.Sample(rngInfo, pathState, hitInfo, wo, wi, f, pdf);
    }

    float NdL = dot(hitInfo.Ns_ff, wi);

    DBG_OUTPUT3(f,                                 BxDF_f);
    DBG_OUTPUT1(pdf,                               BxDF_PDF);
    DBG_OUTPUT3(wi,                                BxDF_L_w);

    return f * abs(NdL) / max(1e-6, pdf);
}

#endif