#ifndef H_FLAGS_METHODS_H
#define H_FLAGS_METHODS_H

#ifndef __cplusplus
#   error C++ only
#endif

#include "PathTracing/Flags/Flags.h"

static constexpr auto s_defaultFeatureFlags = static_cast<PathTracerFeatureFlags>(
        eFeature_Jitter |
        eFeature_Accumulation |
        eFeature_NormalMaps |
        eFeature_EnvironmentMap |
        eFeature_GlassMaterials |
        eFeature_NEE |
        eFeature_FireflyThreshold |
        eFeature_AliasTables |
        //eFeature_RestirDI |
        eFeature_RussianRoulette
    );

static constexpr auto s_defaultFeatureFlagsCbvMode = static_cast<PathTracerFeatureFlags>(
        eFeature_AliasTables |

        eFeature_Jitter |
        eFeature_Accumulation |
        eFeature_EnvironmentMap |
        eFeature_NormalMaps |
        eFeature_DirectionalLight |
        eFeature_DirectionalLightDistant |
        eFeature_DepthOfField |
        eFeature_AlphaTesting |
        eFeature_GlassMaterials |
        eFeature_NEE |
        eFeature_FireflyThreshold |
        eFeature_GammaCorrectionFast |
        eFeature_Anisotropy |
        eFeature_Transient |
        eFeature_ReconstructPrimaryRay |
        eFeature_RestirDI |
        eFeature_RestirTargetVisibility |
        eFeature_GradientDomain |
        eFeature_RussianRoulette
    );

#ifdef _DEBUG
static constexpr auto s_defaultDebugFlags = static_cast<PathTracerDebugFlags>(
        eDebug_NaNTests |
        eDebug_Scales |
        eDebug_ForceLightIndex |
        eDebug_Asserts
    );

static constexpr auto s_defaultDebugFlagsCbvMode = static_cast<PathTracerDebugFlags>(
        eDebug_OutputColor |
        eDebug_OutputColorFindAny |
        eDebug_NaNTests |
        eDebug_ForceSpecular |
        eDebug_ForceDiffuse |
        eDebug_ForceReflect |
        eDebug_ForceRefract |
        eDebug_BxdfTestRevaluate |
        eDebug_BxdfTestHemisphere |
        eDebug_FurnaceTest |
        eDebug_PathDumper |
        eDebug_Scales |
        eDebug_ForceLightIndex |
        eDebug_Asserts
    );
#else
static constexpr auto s_defaultDebugFlags = static_cast<PathTracerDebugFlags>(0);
static constexpr auto s_defaultDebugFlagsCbvMode = static_cast<PathTracerDebugFlags>(0);
#endif

inline bool GetPathTracerFeatureFlag(const PathTracerFeatureFlags state, const PathTracerFeatureFlags flag)
{
    return static_cast<bool>(state & flag);
}

inline bool GetPathTracerFeatureFlag(const int state, const PathTracerFeatureFlags flag)
{
    return static_cast<bool>(state & flag);
}

inline bool GetPathTracerFeatureFlagAtIndex(const PathTracerFeatureFlags state, const int index)
{
    const int flag = 1u << index;
    return static_cast<bool>(state & flag);
}

inline bool GetPathTracerDebugFlag(const PathTracerDebugFlags state, const PathTracerDebugFlags flag)
{
    return static_cast<bool>(state & flag);
}

inline bool GetPathTracerDebugFlag(const int state, const PathTracerDebugFlags flag)
{
    return static_cast<bool>(state & flag);
}

inline bool GetPathTracerDebugFlagAtIndex(const PathTracerDebugFlags state, const int index)
{
    const int flag = 1u << index;
    return static_cast<bool>(state & flag);
}

inline void SetPathTracerFeatureFlag(PathTracerFeatureFlags& state, const PathTracerFeatureFlags flag, const bool enabled)
{
    if (enabled)
        state = static_cast<PathTracerFeatureFlags>(state | flag);
    else if (GetPathTracerFeatureFlag(state, flag))
        state = static_cast<PathTracerFeatureFlags>(state ^ flag);
}

inline void SetPathTracerDebugFlag(PathTracerDebugFlags& state, const PathTracerDebugFlags flag, const bool enabled)
{
    if (enabled)
        state = static_cast<PathTracerDebugFlags>(state | flag);
    else if (GetPathTracerDebugFlag(state, flag))
        state = static_cast<PathTracerDebugFlags>(state ^ flag);
}

// HLSL macros:
#define FEATURE_ENABLED(flag) false
#define FEATURE_ENABLED_PP(flag) false
#define DEBUG_ENABLED(flag)   false
#define DEBUG_ENABLED_PP(flag)   false

#endif