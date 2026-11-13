#ifndef H_NEE_UTILS_H
#define H_NEE_UTILS_H

bool LightIsEnvMap(uint lightIdx)
{
    return lightIdx < gSettings.LsdBasePunctuals;
}
bool LightIsEnvMap(LightSample lightSample)
{
    return LightIsEnvMap(lightSample.Index);
}

bool LightIsPunctual(uint lightIdx)
{
    return lightIdx >= gSettings.LsdBasePunctuals &&
        lightIdx < gSettings.LsdBaseEmissives;
}
bool LightIsPunctual(LightSample lightSample)
{
    return LightIsPunctual(lightSample.Index);
}

bool LightIsEmissive(uint lightIdx)
{
    return lightIdx >= gSettings.LsdBaseEmissives &&
        lightIdx < gSettings.LsdCount;
}
bool LightIsEmissive(LightSample lightSample)
{
    return LightIsEmissive(lightSample.Index);
}

#endif