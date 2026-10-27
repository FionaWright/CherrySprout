#ifndef H_RANDOM_SAMPLE_H
#define H_RANDOM_SAMPLE_H

struct RngValuesPrimary
{
    float2 Jitter;
    float2 DoF;
};

struct RngValuesAncillary
{
    // Batch 1
    float AlphaTest;

    struct RVANEE
    {
        float LSD;
        float2 Light;
    } NEE[MAX_DIRECT_SAMPLES];

    float2 Microfacet;
    float2 LobeSelect;

    // Batch 2
    float RussianRoulette;
};

RngValuesPrimary SamplePrimary(inout RngInfo rngInfo)
{
    RngValuesPrimary r = (RngValuesPrimary)0;

    if (FEAT_CORE(JitterPerPixel))
    {
        r.Jitter.x = Rand01(rngInfo);
        r.Jitter.y = Rand01(rngInfo);
    }

    if (FEAT_CORE(DepthOfField))
    {
        r.DoF.x = Rand01(rngInfo);
        r.DoF.y = Rand01(rngInfo);
    }

    return r;
}

RngValuesAncillary SampleAncillaryBatch1(inout RngInfo rngInfo)
{
    RngValuesAncillary r = (RngValuesAncillary)0;

    if (FEAT_CORE(AlphaTesting))
    {
        r.AlphaTest = Rand01(rngInfo);
    }

    if (FEAT_CORE(NEE))
    {
        for (uint i = 0; i < gSettings.DirectNumSamples; i++)
        {
            r.NEE[i].LSD = Rand01Ex(rngInfo);

            if (FEAT_CORE(Emission) || FEAT_CORE(EnvironmentMap))
            {
                r.NEE[i].Light.x = Rand01Ex(rngInfo);
                r.NEE[i].Light.y = Rand01Ex(rngInfo);
            }
        }
    }

    BxDF bxdf;

    uint mmCount = bxdf.MicrofacetModelRngCount();
    if (mmCount >= 1)
        r.Microfacet.x = Rand01(rngInfo);
    if (mmCount >= 2)
        r.Microfacet.y = Rand01(rngInfo);
    if (mmCount == 3)
        r.Microfacet.z = Rand01(rngInfo);

    uint lobeSelectCount = bxdf.LobeSelectRngCount();
    if (lobeSelectCount >= 1)
        r.LobeSelect.x = Rand01(rngInfo);
    if (lobeSelectCount == 2)
        r.LobeSelect.y = Rand01(rngInfo);

    return r;
}

void SampleAncillaryBatch2(inout RngInfo rngInfo, inout RngValuesAncillary r)
{
    if (FEAT_CORE(RussianRoulette))
    {
        r.RussianRoulette = Rand01(rngInfo);
    }
}

#endif