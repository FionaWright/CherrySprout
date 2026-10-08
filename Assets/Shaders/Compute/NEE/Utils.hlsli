#ifndef H_LSD_UTILS_H
#define H_LSD_UTILS_H

uint GetEnvMapCount()
{
    if (gTotalLums.EnvMapTotalLuminance <= 0.0f)
        return 0;

    return 1;
}

uint GetPunctualsCount()
{
    if (gTotalLums.PunctualTotalLuminance <= 0.0f)
        return 0;

    uint count, _;
    gPunctualPDF.GetDimensions(count, _);

    return count;
}

uint GetEmissivesCount()
{
    if (gTotalLums.EmissiveTotalLuminance <= 0.0f)
        return 0;

    uint count, _;
    gEmissivePDF.GetDimensions(count, _);

    return count;
}

float GetEnvMapWeight(float punctualWeight, float emissiveWeight)
{
    float envWeight = gTotalLums.EnvMapTotalLuminance;

    float nonEnvWeight = punctualWeight + emissiveWeight;
    if (nonEnvWeight <= 0.0f)
        return envWeight;

    return min(nonEnvWeight, envWeight);
}

#endif