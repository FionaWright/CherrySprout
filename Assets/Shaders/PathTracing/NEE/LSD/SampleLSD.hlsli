#ifndef H_SAMPLE_LSD_H
#define H_SAMPLE_LSD_H

#include "PathTracing/NEE/LSD/BinarySearch.hlsli"
#include "PathTracing/NEE/LSD/SampleEnvMapCdf.hlsli"

#if FEATURE_ENABLED_PP(AliasTables)

void sampleLSD(float xi, out uint lightIdx, out float pdf)
{
    xi *= (float)gSettings.LsdCount;
    uint xiIdx = floor(xi);
    float xi01 = frac(xi);

    AliasEntry entry = gLightAliasTable[xiIdx];
    lightIdx = xi01 < entry.Threshold ? xiIdx : entry.Alias;

    pdf = gLightAliasTable[lightIdx].PDF;
}

void EvaluateLSD(uint lightIdx, out float pdf)
{
    pdf = gLightAliasTable[lightIdx].PDF;
}

#else

void sampleLSD(float xi, out uint lightIdx, out float pdf)
{
    lightIdx = BinarySearch(gLightCDF, gSettings.LsdCount, xi);
    pdf = gLightCDF[lightIdx].PDF;
}

void EvaluateLSD(uint lightIdx, out float pdf)
{
    pdf = gLightCDF[lightIdx].PDF;
}

#endif

void SampleLSD(inout RngInfo rngInfo, out uint lightIdx, out float pdf)
{
    if (DEBUG_ENABLED(ForceLightIndex) && gDebugSettings.ForcedLightIndex >= 0)
    {
        lightIdx = gDebugSettings.ForcedLightIndex;
        pdf = 1.0f;
        return;
    }

    float xi = Rand01Ex(rngInfo);
    sampleLSD(xi, lightIdx, pdf);

    DBG_OUTPUT1(xi, NEE_xi);
}

#endif