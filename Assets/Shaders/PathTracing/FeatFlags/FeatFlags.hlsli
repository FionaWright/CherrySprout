#ifndef H_FLAGS_METHODS_HLSL_H
#define H_FLAGS_METHODS_HLSL_H

#ifdef __cplusplus
#   error HLSL only
#endif

#include "PathTracing/FeatFlags/Combinations.hlsli"

#if defined(FEAT_FLAGS_COMP_CORE) && defined(FEAT_FLAGS_COMP_DBG) && defined(FEAT_FLAGS_COMP_ADV)

#include "PathTracing/FeatFlags/Internal/Interpretations.h"

#define FEAT_CORE_D(flag)   (FEAT_FLAGS_COMP_CORE & FEAT_FLAG_VALUE_CORE_##flag)
#define FEAT_ADV_D(flag)    (FEAT_FLAGS_COMP_ADV & FEAT_FLAG_VALUE_ADV_##flag)
#define FEAT_DBG_D(flag)    (FEAT_FLAGS_COMP_DBG & FEAT_FLAG_VALUE_DBG_##flag)

#define CHECK_RUNTIME_FLAGS (FF_QUICK_SWITCH_ENABLED && defined(PT_BUFFERS_DEFINED))

#if CHECK_RUNTIME_FLAGS

bool featFlagEnabledCore(uint flagValue, uint linearIndex)
{
    bool isQuickSwitchable = s_featFlagsIsQsCORE[linearIndex];
    if (!isQuickSwitchable)
        return true;
    return gDebugSettings.FeatFlagsCoreQS & flagValue;
}
#    define FEAT_CORE(flag) (FEAT_CORE_D(flag) && featFlagEnabledCore(FEAT_FLAG_VALUE_CORE_##flag, (hlsl::uint)Internal_FeatFlagIndicesCore::Idx_##flag))

bool featFlagEnabledAdv(uint flagValue, uint linearIndex)
{
    bool isQuickSwitchable = s_featFlagsIsQsADV[linearIndex];
    if (!isQuickSwitchable)
        return true;
    return gDebugSettings.FeatFlagsAdvQS & flagValue;
}
#    define FEAT_ADV(flag) (FEAT_ADV_D(flag) && featFlagEnabledAdv(FEAT_FLAG_VALUE_ADV_##flag, (hlsl::uint)Internal_FeatFlagIndicesAdv::Idx_##flag))

bool featFlagEnabledDbg(uint flagValue, uint linearIndex)
{
    bool isQuickSwitchable = s_featFlagsIsQsDBG[linearIndex];
    if (!isQuickSwitchable)
        return true;
    return gDebugSettings.FeatFlagsDbgQS & flagValue;
}
#    define FEAT_DBG(flag)   (FEAT_DBG_D(flag) && featFlagEnabledDbg(FEAT_FLAG_VALUE_DBG_##flag, (hlsl::uint)Internal_FeatFlagIndicesDbg::Idx_##flag))

#else
#    define FEAT_CORE(flag)  FEAT_CORE_D(flag)
#    define FEAT_ADV(flag)   FEAT_ADV_D(flag)
#    define FEAT_DBG(flag)   FEAT_DBG_D(flag)
#endif

#else

#define FEAT_CORE_D(flag) (false)
#define FEAT_ADV_D(flag) (false)
#define FEAT_DBG_D(flag) (false)

#define FEAT_CORE(flag) (false)
#define FEAT_ADV(flag) (false)
#define FEAT_DBG(flag) (false)

#endif

#endif