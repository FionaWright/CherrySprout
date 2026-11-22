#ifndef H_FLAGS_METHODS_H
#define H_FLAGS_METHODS_H

#ifndef __cplusplus
#   error C++ only
#endif

#include "Internal/Interpretations.h"

static constexpr auto s_defaultFeatFlagsCORE = static_cast<FeatFlagsCORE>(
        //eCORE_JitterPerPixel |
        //eCORE_JitterPerFrame |
        eCORE_Accumulation |
        eCORE_NormalMaps |
        //eCORE_EnvironmentMap |
        //eCORE_Transmission |
        eCORE_NEE |
        //eCORE_FireflyThreshold |
        //eCORE_Emission |
        eCORE_Punctuals |
        eCORE_AliasTables |
        //eCORE_RussianRoulette
        eCORE_AlphaTesting
    );

static constexpr auto s_defaultFeatFlagsCOREQS = static_cast<FeatFlagsCORE>(~0);

static constexpr auto s_defaultFeatFlagsADV = static_cast<FeatFlagsADV>(
        //eADV_GradientDomain |
        eADV_GdptShiftRecEnv |
        eADV_GdptShiftRec |
        eADV_GdptShiftHalf |
        eADV_HybridShift |
        eADV_RestirDI |
        //eADV_RestirGI |
        eADV_RestirTargetVisibility |
        //eADV_RestirPT |
        eADV_AreaRestir
    );

static constexpr auto s_defaultFeatFlagsADVQS = static_cast<FeatFlagsADV>(~0);

#if !NDEBUG
static constexpr auto s_defaultFeatFlagsDBG = static_cast<FeatFlagsDBG>(
        eDBG_NaNTests |
        eDBG_Scales |
        eDBG_ForceLightIndex |
        //eDBG_GeometricNormalOnly |
        //eDBG_PathDumper |
        eDBG_OutputColor |
        eDBG_Asserts
    );

static constexpr auto s_defaultFeatFlagsDBGQS = static_cast<FeatFlagsDBG>(~0);
#else
static constexpr auto s_defaultFeatFlagsDBG = static_cast<FeatFlagsDBG>(0);
static constexpr auto s_defaultFeatFlagsDBGQS = static_cast<FeatFlagsDBG>(0);
#endif

template <typename TFeatFlags>
bool GetFeatFlag(const TFeatFlags state, const TFeatFlags flag)
{
    return static_cast<bool>(state & flag);
}

template <typename TFeatFlags>
bool GetFeatFlagAtIdx(const TFeatFlags state, const int index)
{
    const int flag = 1u << index;
    return static_cast<bool>(state & flag);
}

template <typename TFeatFlags>
void SetFeatFlag(TFeatFlags& state, const TFeatFlags flag, const bool enabled)
{
    if (enabled)
        state = static_cast<TFeatFlags>(state | flag);
    else if (GetFeatFlag(state, flag))
        state = static_cast<TFeatFlags>(state ^ flag);
}

// HLSL macros:
#define FEAT_CORE(flag) false
#define FEAT_CORE_D(flag) false
#define FEAT_ADV(flag) false
#define FEAT_ADV_D(flag) false
#define FEAT_DBG(flag)   false
#define FEAT_DBG_D(flag)   false

#endif