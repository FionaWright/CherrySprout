#ifndef H_FLAGS_METHODS_HLSL_H
#define H_FLAGS_METHODS_HLSL_H

#ifdef __cplusplus
#   error HLSL only
#endif

#include "PathTracing/Flags/Internal/Flags.h"

#   ifndef FEATURE_FLAGS
#       define FEATURE_FLAGS 0
#   endif

#   ifndef DEBUG_FLAGS
#       define DEBUG_FLAGS 0
#   endif

#define FEATURE_ENABLED_PP(flag) (FEATURE_FLAGS & FEATURE_FLAG_VALUE_##flag)
#define DEBUG_ENABLED_PP(flag)   (DEBUG_FLAGS & DEBUG_FLAG_VALUE_##flag)

#define CHECK_RUNTIME_FLAGS (DEBUG_CBV_FLAGS_MODE_ENABLED && defined(PT_BUFFERS_DEFINED))

#if CHECK_RUNTIME_FLAGS

bool featureEnabled(uint flagValue, uint linearIndex)
{
    bool canBeCbvValue = s_canBeCbvValueFlagListFeature[linearIndex];
    return ((FEATURE_FLAGS & flagValue) &&                         // Comptime enabled
        canBeCbvValue &&                                           // Has runtime functionality
        (gDebugSettings.CbvFeatureFlags & flagValue));             // Runtime enabled
}
#    define FEATURE_ENABLED(flag) (featureEnabled(FEATURE_FLAG_VALUE_##flag, (hlsl::uint)Internal_PathTracerFeatureIndex::Idx_##flag))

bool debugEnabled(uint flagValue, uint linearIndex)
{
    bool canBeCbvValue = s_canBeCbvValueFlagListDebug[linearIndex];
    return ((DEBUG_FLAGS & flagValue) &&                          // Comptime enabled
        canBeCbvValue &&                                          // Has runtime functionality
        (gDebugSettings.CbvDebugFlags & flagValue));              // Runtime enabled
}
#    define DEBUG_ENABLED(flag)   (debugEnabled(DEBUG_FLAG_VALUE_##flag, (hlsl::uint)Internal_PathTracerDebugIndex::Idx_##flag))

#else
#    define FEATURE_ENABLED(flag) FEATURE_ENABLED_PP(flag)
#    define DEBUG_ENABLED(flag)   DEBUG_ENABLED_PP(flag)
#endif

#endif