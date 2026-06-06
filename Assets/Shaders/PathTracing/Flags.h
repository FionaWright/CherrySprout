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

inline bool GetPathTracerFeatureFlag(const int state, const PathTracerFeatureFlags flag)
{
    return bool(state & flag);
}

inline bool GetPathTracerDebugFlag(const PathTracerDebugFlags state, const PathTracerDebugFlags flag)
{
    return bool(state & flag);
}

inline bool GetPathTracerDebugFlag(const int state, const PathTracerDebugFlags flag)
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

static constexpr PathTracerFeatureFlags s_defaultFeatureFlags = eFeature_Accumulation;
static constexpr PathTracerDebugFlags s_defaultDebugFlags = eDebug_None;

inline void SetPathTracerFeatureFlag(PathTracerFeatureFlags& state, const PathTracerFeatureFlags flag, const bool enabled)
{
    if (enabled)
        state = PathTracerFeatureFlags(state | flag);
    else if (GetPathTracerFeatureFlag(state, flag))
        state = PathTracerFeatureFlags(state ^ flag);
}

inline void SetPathTracerDebugFlag(PathTracerDebugFlags& state, const PathTracerDebugFlags flag, const bool enabled)
{
    if (enabled)
        state = PathTracerDebugFlags(state | flag);
    else if (GetPathTracerDebugFlag(state, flag))
        state = PathTracerDebugFlags(state ^ flag);
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