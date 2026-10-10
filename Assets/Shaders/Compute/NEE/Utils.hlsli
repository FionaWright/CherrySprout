#ifndef H_LSD_UTILS_H
#define H_LSD_UTILS_H

uint GetEnvMapCount()
{
    if (gCBV.EnvMapTotalLuminance <= 0.0f)
        return 0;

    return 1;
}

uint GetPunctualsCount()
{
    if (gCBV.PunctualTotalLuminance <= 0.0f)
        return 0;

    return gCBV.PunctualCount;
}

uint GetEmissivesCount()
{
    if (gCBV.EmissiveTotalLuminance <= 0.0f)
        return 0;

    return gCBV.EmissiveCount;
}

float GetEnvMapWeight(float punctualWeight, float emissiveWeight)
{
    float envWeight = gCBV.EnvMapTotalLuminance;

    float nonEnvWeight = punctualWeight + emissiveWeight;
    if (nonEnvWeight <= 0.0f)
        return envWeight;

    return min(nonEnvWeight, envWeight);
}

#endif