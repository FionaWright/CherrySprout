#ifndef H_PT_FLAGS_H
#define H_PT_FLAGS_H

#include "Utils/HlslGlue.h"

#define FEATURE_FLAG_TYPE hlsl::uint
#define DEBUG_FLAG_TYPE hlsl::uint

enum class Internal_PathTracerFeatureIndex : hlsl::uint
{
    Idx_Jitter,
    Idx_DepthOfField,
    Idx_Accumulation,
    Idx_EnvironmentMap,
    Idx_EnvironmentMapEA,
    Idx_RussianRoulette,
    Idx_NormalMaps,
    Idx_AlphaTesting,
    Idx_GlassMaterials,

    INTERNAL_FEATURE_COUNT
};

enum PathTracerFeatureFlags : FEATURE_FLAG_TYPE
{
    eFeature_None               = 0,

    eFeature_Jitter             = 1u << (hlsl::uint)Internal_PathTracerFeatureIndex::Idx_Jitter,
    eFeature_DepthOfField       = 1u << (hlsl::uint)Internal_PathTracerFeatureIndex::Idx_DepthOfField,
    eFeature_Accumulation       = 1u << (hlsl::uint)Internal_PathTracerFeatureIndex::Idx_Accumulation,
    eFeature_EnvironmentMap     = 1u << (hlsl::uint)Internal_PathTracerFeatureIndex::Idx_EnvironmentMap,
    eFeature_EnvironmentMapEA   = 1u << (hlsl::uint)Internal_PathTracerFeatureIndex::Idx_EnvironmentMapEA,
    eFeature_RussianRoulette    = 1u << (hlsl::uint)Internal_PathTracerFeatureIndex::Idx_RussianRoulette,
    eFeature_NormalMaps         = 1u << (hlsl::uint)Internal_PathTracerFeatureIndex::Idx_NormalMaps,
    eFeature_AlphaTesting       = 1u << (hlsl::uint)Internal_PathTracerFeatureIndex::Idx_AlphaTesting,
    eFeature_GlassMaterials     = 1u << (hlsl::uint)Internal_PathTracerFeatureIndex::Idx_GlassMaterials,

    FEATURE_COUNT               =       (hlsl::uint)Internal_PathTracerFeatureIndex::INTERNAL_FEATURE_COUNT
};

enum class Internal_PathTracerDebugIndex : hlsl::uint
{
    Idx_FurnaceTest,
    Idx_OutputColor,
    Idx_NaNTests,

    INTERNAL_DEBUG_COUNT
};

enum PathTracerDebugFlags : DEBUG_FLAG_TYPE
{
    eDebug_None               = 0,

    eDebug_FurnaceTest        = 1u << (hlsl::uint)Internal_PathTracerDebugIndex::Idx_FurnaceTest,
    eDebug_OutputColor        = 1u << (hlsl::uint)Internal_PathTracerDebugIndex::Idx_OutputColor,
    eDebug_NaNTests           = 1u << (hlsl::uint)Internal_PathTracerDebugIndex::Idx_NaNTests,

    DEBUG_COUNT               =       (hlsl::uint)Internal_PathTracerDebugIndex::INTERNAL_DEBUG_COUNT
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
    "Jitter",
    "Depth Of Field",
    "Accumulation",
    "Environment Map",
    "Environment Map Equal-Area",
    "Russian Roulette",
    "Normal Maps",
    "Alpha Testing",
    "Glass materials"
};
static const char* s_debugFlagNames[DEBUG_COUNT] = {
    "Furnace Test",
    "Output Color",
    "NaN Tests",
};

static constexpr PathTracerFeatureFlags s_defaultFeatureFlags = static_cast<PathTracerFeatureFlags>(
        eFeature_Accumulation | eFeature_EnvironmentMap | eFeature_EnvironmentMapEA | eFeature_NormalMaps
    );
static constexpr PathTracerDebugFlags s_defaultDebugFlags = static_cast<PathTracerDebugFlags>(
        eDebug_NaNTests
    );

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

#define FEATURE_ENABLED(flag)   GetPathTracerFeatureFlag(FEATURE_FLAGS, eFeature_##flag)
#define DEBUG_ENABLED(flag)     GetPathTracerDebugFlag  (DEBUG_FLAGS,   eDebug_##flag)

#endif

#endif