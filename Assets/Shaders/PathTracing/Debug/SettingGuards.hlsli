#ifndef H_SETTING_GUARDS_H
#define H_SETTING_GUARDS_H

float GetFixedMisNee()
{
#if FEAT_DBG_D(FixedMisNeeManual)
    return gDebugSettings.FixedMisNee;
#else
    return NAN;
#endif
}

int GetForcedLightIndex()
{
#if FEAT_DBG_D(ForceLightIndex)
    return gDebugSettings.ForcedLightIndex;
#else
    return -1;
#endif
}

#endif