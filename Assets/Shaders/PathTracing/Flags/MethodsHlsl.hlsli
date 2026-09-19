#ifndef H_FLAGS_METHODS_HLSL_H
#define H_FLAGS_METHODS_HLSL_H

#ifdef __cplusplus
#   error HLSL only
#endif

#include "PathTracing/Flags/Flags.h"

#   ifndef FEATURE_FLAGS
#       define FEATURE_FLAGS 0
#   endif

#   ifndef DEBUG_FLAGS
#       define DEBUG_FLAGS 0
#   endif

bool featureEnabled(uint flagValue, uint linearIndex)
{
#if DEBUG_CBV_FLAGS_MODE_ENABLED && defined(PT_BUFFERS_DEFINED)
    bool canBeCbvValue = s_canBeCbvValueFlagListFeature[linearIndex];
    return ((FEATURE_FLAGS & flagValue) &&                         // Comptime enabled
        canBeCbvValue &&                                       // Has runtime functionality
        (gDebugSettings.CbvFeatureFlags & flagValue));          // Runtime enabled
#else
    return FEATURE_FLAGS & flagValue;
#endif
}

bool debugEnabled(uint flagValue, uint linearIndex)
{
#if DEBUG_CBV_FLAGS_MODE_ENABLED && defined(PT_BUFFERS_DEFINED)
    bool canBeCbvValue = s_canBeCbvValueFlagListDebug[linearIndex];
    return ((DEBUG_FLAGS & flagValue) &&                          // Comptime enabled
        canBeCbvValue &&                                      // Has runtime functionality
        (gDebugSettings.CbvDebugFlags & flagValue));           // Runtime enabled
#else
    return DEBUG_FLAGS & flagValue;
#endif
}

#define FEATURE_ENABLED_PP(flag) (FEATURE_FLAGS & FEATURE_FLAG_VALUE_##flag)
#define DEBUG_ENABLED_PP(flag)   (DEBUG_FLAGS & DEBUG_FLAG_VALUE_##flag)

#define FEATURE_ENABLED(flag) (featureEnabled(FEATURE_FLAG_VALUE_##flag, (hlsl::uint)Internal_PathTracerFeatureIndex::Idx_##flag))
#define DEBUG_ENABLED(flag)   (debugEnabled(DEBUG_FLAG_VALUE_##flag, (hlsl::uint)Internal_PathTracerDebugIndex::Idx_##flag))

#endif