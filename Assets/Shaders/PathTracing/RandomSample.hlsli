#ifndef H_RANDOM_SAMPLE_H
#define H_RANDOM_SAMPLE_H

#include "PathTracing/RandomSampleStructs.hlsli"
#include "Utils/Random.h"
#include "BxDFs/GetBxDF.hlsli"

RngValuesPrimary SampleRngPrimary(inout RngInfo rngInfo)
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

RngValuesAncillary::RngValuesNEE SampleRngNEE(inout RngInfo rngInfo)
{
    RngValuesAncillary::RngValuesNEE r = (RngValuesAncillary::RngValuesNEE)0;

    r.LSD = Rand01Ex(rngInfo);

    if (FEAT_CORE(Emission) || FEAT_CORE(EnvironmentMap))
    {
        r.Light.x = Rand01Ex(rngInfo);
        r.Light.y = Rand01Ex(rngInfo);
    }

    return r;
}

RngValuesAncillary::RngValuesBxDF SampleRngBxDF(inout RngInfo rngInfo)
{
    RngValuesAncillary::RngValuesBxDF r = (RngValuesAncillary::RngValuesBxDF)0;

    BxDF bxdf;
    uint mmCount = bxdf.MicrofacetModelRngCount();
    uint lobeSelectCount = bxdf.LobeSelectRngCount();

    if (mmCount >= 1)
        r.Microfacet.x = Rand01(rngInfo);
    if (mmCount >= 2)
        r.Microfacet.y = Rand01(rngInfo);
    if (mmCount == 3)
        r.Microfacet.z = Rand01(rngInfo);

    if (lobeSelectCount >= 1)
        r.LobeSelect.x = Rand01(rngInfo);
    if (lobeSelectCount == 2)
        r.LobeSelect.y = Rand01(rngInfo);

    return r;
}

RngValuesAncillary SampleRngAncillaryBatch1(inout RngInfo rngInfo)
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
            r.NEE[i] = SampleRngNEE(rngInfo);
        }
    }

    r.BxDF = SampleRngBxDF(rngInfo);

    return r;
}

void SampleRngAncillaryBatch2(inout RngInfo rngInfo, inout RngValuesAncillary r)
{
    if (FEAT_CORE(RussianRoulette))
    {
        r.RussianRoulette = Rand01(rngInfo);
    }
}

#endif