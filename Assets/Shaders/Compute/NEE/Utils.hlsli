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
    float realLum = gTotalLums.EnvMapTotalLuminance / 10000;

    bool anyNonEnvWeight = punctualWeight > 0.0f || emissiveWeight > 0.0f;
    if (!anyNonEnvWeight)
        return realLum;

    float totalWeight = punctualWeight + emissiveWeight + realLum;
    float cappedLum = 0.5f * totalWeight;

    return clamp(realLum, 0.0f, cappedLum);
}

#endif