#ifndef H_SAMPLE_LSD_H
#define H_SAMPLE_LSD_H

#include "PathTracing/NEE/LSD/BinarySearch.hlsli"
#include "PathTracing/NEE/LSD/SampleEnvMapCdf.hlsli"

#if FEAT_CORE_D(AliasTables)

void SampleLSD(float xi, out uint lightIdx, out float pdf)
{
    if (FEAT_DBG(ForceLightIndex) && gDebugSettings.ForcedLightIndex >= 0)
    {
        lightIdx = gDebugSettings.ForcedLightIndex;
        pdf = 1.0f;
        return;
    }

    xi *= (float)gSettings.LsdCount;
    uint xiIdx = floor(xi);
    float xi01 = frac(xi);

    AliasEntry entry = gLightAliasTable[xiIdx];
    lightIdx = xi01 < entry.Threshold ? xiIdx : entry.Alias;

    pdf = gLightAliasTable[lightIdx].PDF;
}

void EvaluateLSD(uint lightIdx, out float pdf)
{
    if (FEAT_DBG(ForceLightIndex) && gDebugSettings.ForcedLightIndex >= 0)
    {
        pdf = 1.0f;
        return;
    }

    pdf = gLightAliasTable[lightIdx].PDF;
}

#else

void SampleLSD(float xi, out uint lightIdx, out float pdf)
{
    if (FEAT_DBG(ForceLightIndex) && gDebugSettings.ForcedLightIndex >= 0)
    {
        lightIdx = gDebugSettings.ForcedLightIndex;
        pdf = 1.0f;
        return;
    }

    lightIdx = BinarySearch(gLightCDF, gSettings.LsdCount, xi);
    pdf = gLightCDF[lightIdx].PDF;
}

void EvaluateLSD(uint lightIdx, out float pdf)
{
    if (FEAT_DBG(ForceLightIndex) && gDebugSettings.ForcedLightIndex >= 0)
    {
        pdf = 1.0f;
        return;
    }

    pdf = gLightCDF[lightIdx].PDF;
}

#endif

#endif