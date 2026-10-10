#ifndef H_FLAGS_METHODS_H
#define H_FLAGS_METHODS_H

#ifndef __cplusplus
#   error C++ only
#endif

#include "Internal/Interpretations.h"

static constexpr auto s_defaultFeatFlagsCore = static_cast<FeatFlagsCore>(
        eCore_Jitter |
        eCore_Accumulation |
        eCore_NormalMaps |
        eCore_EnvironmentMap |
        eCore_AlphaTesting |
        //eCore_Transmission |
        eCore_NEE |
        //eCore_FireflyThreshold |
        eCore_Emission |
        //eCore_GradientDomain |
        //eCore_ScreenSpaceGradients |
        eCore_AliasTables
        //eCore_RestirDI |
        //eCore_RussianRoulette
    );

static constexpr auto s_defaultFeatFlagsCoreQS = static_cast<FeatFlagsCore>(~0);

#if !NDEBUG
static constexpr auto s_defaultFeatFlagsDbg = static_cast<FeatFlagsDbg>(
        eDebug_NaNTests |
        eDebug_Scales |
        eDebug_ForceLightIndex |
        //eDebug_PathDumper |
        eDebug_Asserts
    );

static constexpr auto s_defaultFeatFlagsDbgQS = static_cast<FeatFlagsDbg>(~0);
#else
static constexpr auto s_defaultFeatFlagsDbg = static_cast<FeatFlagsDbg>(0);
static constexpr auto s_defaultFeatFlagsDbgQS = static_cast<FeatFlagsDbg>(0);
#endif

inline bool GetFeatFlagCore(const FeatFlagsCore state, const FeatFlagsCore flag)
{
    return static_cast<bool>(state & flag);
}

inline bool GetFeatFlagCore(const int state, const FeatFlagsCore flag)
{
    return static_cast<bool>(state & flag);
}

inline bool GetFeatFlagAtIdxCore(const FeatFlagsCore state, const int index)
{
    const int flag = 1u << index;
    return static_cast<bool>(state & flag);
}

inline bool GetFeatFlagDebug(const FeatFlagsDbg state, const FeatFlagsDbg flag)
{
    return static_cast<bool>(state & flag);
}

inline bool GetFeatFlagDebug(const int state, const FeatFlagsDbg flag)
{
    return static_cast<bool>(state & flag);
}

inline bool GetFeatFlagAtIdxDebug(const FeatFlagsDbg state, const int index)
{
    const int flag = 1u << index;
    return static_cast<bool>(state & flag);
}

inline void SetFeatFlagCore(FeatFlagsCore& state, const FeatFlagsCore flag, const bool enabled)
{
    if (enabled)
        state = static_cast<FeatFlagsCore>(state | flag);
    else if (GetFeatFlagCore(state, flag))
        state = static_cast<FeatFlagsCore>(state ^ flag);
}

inline void SetFeatFlagDebug(FeatFlagsDbg& state, const FeatFlagsDbg flag, const bool enabled)
{
    if (enabled)
        state = static_cast<FeatFlagsDbg>(state | flag);
    else if (GetFeatFlagDebug(state, flag))
        state = static_cast<FeatFlagsDbg>(state ^ flag);
}

// HLSL macros:
#define FEAT_CORE(flag) false
#define FEAT_CORE_D(flag) false
#define FEAT_DBG(flag)   false
#define FEAT_DBG_D(flag)   false

#endif