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
    Idx_Anisotropy,

    INTERNAL_FEATURE_COUNT
};
#define __FEATURE_IDX(x) (hlsl::uint)Internal_PathTracerFeatureIndex::Idx_##x

enum PathTracerFeatureFlags : FEATURE_FLAG_TYPE
{
    eFeature_None               = 0,

    eFeature_Jitter             = 1u << __FEATURE_IDX(Jitter),
    eFeature_DepthOfField       = 1u << __FEATURE_IDX(DepthOfField),
    eFeature_Accumulation       = 1u << __FEATURE_IDX(Accumulation),
    eFeature_EnvironmentMap     = 1u << __FEATURE_IDX(EnvironmentMap),
    eFeature_EnvironmentMapEA   = 1u << __FEATURE_IDX(EnvironmentMapEA),
    eFeature_RussianRoulette    = 1u << __FEATURE_IDX(RussianRoulette),
    eFeature_NormalMaps         = 1u << __FEATURE_IDX(NormalMaps),
    eFeature_AlphaTesting       = 1u << __FEATURE_IDX(AlphaTesting),
    eFeature_GlassMaterials     = 1u << __FEATURE_IDX(GlassMaterials),
    eFeature_Anisotropy         = 1u << __FEATURE_IDX(Anisotropy),

    FEATURE_COUNT = (hlsl::uint)Internal_PathTracerFeatureIndex::INTERNAL_FEATURE_COUNT
};

enum class Internal_PathTracerDebugIndex : hlsl::uint
{
    Idx_FurnaceTest,
    Idx_OutputColor,
    Idx_NaNTests,
    Idx_Asserts,
    Idx_ForceSpecular,
    Idx_ForceDiffuse,
    Idx_ForceReflect,
    Idx_ForceRefract,

    INTERNAL_DEBUG_COUNT
};
#define __DBG_IDX(x) (hlsl::uint)Internal_PathTracerDebugIndex::Idx_##x

enum PathTracerDebugFlags : DEBUG_FLAG_TYPE
{
    eDebug_None               = 0,

    eDebug_FurnaceTest              = 1u << __DBG_IDX(FurnaceTest),
    eDebug_OutputColor              = 1u << __DBG_IDX(OutputColor),
    eDebug_NaNTests                 = 1u << __DBG_IDX(NaNTests),
    eDebug_Asserts                  = 1u << __DBG_IDX(Asserts),
    eDebug_ForceSpecular            = 1u << __DBG_IDX(ForceSpecular),
    eDebug_ForceDiffuse             = 1u << __DBG_IDX(ForceDiffuse),
    eDebug_ForceReflect             = 1u << __DBG_IDX(ForceReflect),
    eDebug_ForceRefract             = 1u << __DBG_IDX(ForceRefract),

    DEBUG_COUNT = (hlsl::uint)Internal_PathTracerDebugIndex::INTERNAL_DEBUG_COUNT
};

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
    "Glass materials",
    "Anisotropy",
};
static const char* s_debugFlagNames[DEBUG_COUNT] = {
    "Furnace Test",
    "Output Color",
    "NaN Tests",
    "Asserts",
    "Force Specular",
    "Force Diffuse",
    "Force Reflect",
    "Force Refract",
};

static constexpr PathTracerFeatureFlags s_defaultFeatureFlags = static_cast<PathTracerFeatureFlags>(
        eFeature_Jitter |
        eFeature_Accumulation |
        eFeature_EnvironmentMap |
        eFeature_EnvironmentMapEA |
        eFeature_NormalMaps |
        eFeature_GlassMaterials |
        eFeature_RussianRoulette
    );
static constexpr PathTracerDebugFlags s_defaultDebugFlags = static_cast<PathTracerDebugFlags>(
        eDebug_NaNTests
    );

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

#else

#   ifndef FEATURE_FLAGS
#       define FEATURE_FLAGS eFeature_None
#   endif

#   ifndef DEBUG_FLAGS
#       define DEBUG_FLAGS eDebug_None
#   endif

#define FEATURE_ENABLED(flag)  (FEATURE_FLAGS   &   eFeature_##flag)
#define DEBUG_ENABLED(flag)    (DEBUG_FLAGS     &   eDebug_##flag)

#endif

#endif