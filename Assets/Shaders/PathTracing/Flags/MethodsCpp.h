#ifndef H_FLAGS_METHODS_H
#define H_FLAGS_METHODS_H

#ifndef __cplusplus
#   error C++ only
#endif

#include "PathTracing/Flags/Flags.h"

static constexpr PathTracerFeatureFlags s_defaultFeatureFlags = static_cast<PathTracerFeatureFlags>(
        eFeature_Jitter |
        eFeature_Accumulation |
        eFeature_EnvironmentMap |
        eFeature_NormalMaps |
        eFeature_GlassMaterials |
        eFeature_NEE |
        eFeature_FireflyThreshold |
        eFeature_RussianRoulette
    );

#ifdef _DEBUG
static constexpr PathTracerDebugFlags s_defaultDebugFlags = static_cast<PathTracerDebugFlags>(
        eDebug_NaNTests |
        eDebug_Asserts
    );
#else
static constexpr PathTracerDebugFlags s_defaultDebugFlags = static_cast<PathTracerDebugFlags>(0);
#endif

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

#endif