#ifndef H_PT_FLAGS_H
#define H_PT_FLAGS_H

#include "Utils/HlslGlue.h"

#define FEATURE_FLAG_TYPE hlsl::uint
#define DEBUG_FLAG_TYPE hlsl::uint

enum PathTracerFeatureFlags : FEATURE_FLAG_TYPE
{
    eFeature_None               = 0,
    eFeature_Jitter             = 1 << 0,
    eFeature_DepthOfField       = 1 << 1,
    eFeature_Accumulation       = 1 << 2,
    eFeature_EnvironmentMap     = 1 << 3,
    eFeature_RussianRoulette    = 1 << 4,
    eFeature_NormalMaps         = 1 << 5,
    eFeature_AlphaTesting       = 1 << 6,
    eFeature_GlassMaterials     = 1 << 7,
    FEATURE_COUNT               =      8
};

enum PathTracerDebugFlags : DEBUG_FLAG_TYPE
{
    eDebug_None               = 0,
    eDebug_FurnaceTest        = 1 << 0,
    eDebug_OutputColor        = 1 << 1,
    DEBUG_COUNT               =      2
};

inline bool GetPathTracerFeatureFlag(const PathTracerFeatureFlags state, const PathTracerFeatureFlags flag)
{
    return bool(state & flag);
}

inline bool GetPathTracerDebugFlag(const PathTracerDebugFlags state, const PathTracerDebugFlags flag)
{
    return bool(state & flag);
}

#ifdef __cplusplus

static const char* s_featureFlagNames[FEATURE_COUNT] = {
    "Jitter", "Depth Of Field", "Accumulation", "Environment Map", "Russian Roulette", "Normal Maps", "Alpha Testing", "Glass materials"
};
static const char* s_debugFlagNames[DEBUG_COUNT] = {
    "Furnace Test", "Output Color"
};

inline PathTracerFeatureFlags operator&(PathTracerFeatureFlags lhs, PathTracerFeatureFlags rhs)
{
    const FEATURE_FLAG_TYPE combined = FEATURE_FLAG_TYPE(lhs) & FEATURE_FLAG_TYPE(rhs);
    return PathTracerFeatureFlags(combined);
}

inline PathTracerDebugFlags operator&(PathTracerDebugFlags lhs, PathTracerDebugFlags rhs)
{
    const DEBUG_FLAG_TYPE combined = DEBUG_FLAG_TYPE(lhs) & DEBUG_FLAG_TYPE(rhs);
    return PathTracerDebugFlags(combined);
}

inline PathTracerFeatureFlags operator|(PathTracerFeatureFlags lhs, PathTracerFeatureFlags rhs)
{
    const FEATURE_FLAG_TYPE combined = FEATURE_FLAG_TYPE(lhs) | FEATURE_FLAG_TYPE(rhs);
    return PathTracerFeatureFlags(combined);
}

inline PathTracerFeatureFlags operator^(PathTracerFeatureFlags lhs, PathTracerFeatureFlags rhs)
{
    const FEATURE_FLAG_TYPE combined = FEATURE_FLAG_TYPE(lhs) ^ FEATURE_FLAG_TYPE(rhs);
    return PathTracerFeatureFlags(combined);
}

inline PathTracerDebugFlags operator|(PathTracerDebugFlags lhs, PathTracerDebugFlags rhs)
{
    const DEBUG_FLAG_TYPE combined = DEBUG_FLAG_TYPE(lhs) | DEBUG_FLAG_TYPE(rhs);
    return PathTracerDebugFlags(combined);
}

inline PathTracerDebugFlags operator^(PathTracerDebugFlags lhs, PathTracerDebugFlags rhs)
{
    const DEBUG_FLAG_TYPE combined = DEBUG_FLAG_TYPE(lhs) ^ DEBUG_FLAG_TYPE(rhs);
    return PathTracerDebugFlags(combined);
}

inline void SetPathTracerFeatureFlag(PathTracerFeatureFlags& state, const PathTracerFeatureFlags flag, const bool enabled)
{
    if (enabled)
        state = state | flag;
    else if (GetPathTracerFeatureFlag(state, flag))
        state = state ^ flag;
}

inline void SetPathTracerDebugFlag(PathTracerDebugFlags& state, const PathTracerDebugFlags flag, const bool enabled)
{
    if (enabled)
        state = state | flag;
    else if (GetPathTracerDebugFlag(state, flag))
        state = state ^ flag;
}

inline std::string ToString(PathTracerFeatureFlags flags)
{
    return std::to_string(static_cast<FEATURE_FLAG_TYPE>(flags));
}

inline std::string ToString(PathTracerDebugFlags flags)
{
    return std::to_string(static_cast<DEBUG_FLAG_TYPE>(flags));
}

#else

#   ifndef FEATURE_FLAGS
#       define FEATURE_FLAGS eFeature_None
#   endif

#   ifndef DEBUG_FLAGS
#       define DEBUG_FLAGS eDebug_None
#   endif

#endif

#endif