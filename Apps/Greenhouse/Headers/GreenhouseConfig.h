#ifndef H_GREENHOUSE_CONFIG_H
#define H_GREENHOUSE_CONFIG_H

#include "PathTracer.h"
#include "BxDFs/BxDFMode.h"
#include "MicrofacetModels/MMTypes.h"
#include "../../../Assets/Shaders/PathTracing/Debug/Internal/OutputColor.h"
#include "PathTracing/Debug/OutputColorRemap.h"
#include "../../../Assets/Shaders/PathTracing/FeatFlags/FeatFlags.hpp"
#include "MicrofacetModels/MMTypes.h"

constexpr hlsl::uint2 s_defaultChosenPixelIdx = hlsl::uint2(300, 300);

struct PathTracingDebugInfo
{
#if CHERRY_DEBUG_FEATURES_ENABLED
    FeatFlagsDbg FlagsDebug = s_defaultFeatFlagsDbg;
    DebugOutputIndex OutputColorIdx = s_defaultOutputIndex;
    DebugOutputColorRemap OutputColorRemap = s_defaultOutputColorRemap;
    int ChosenRayDepth = -1;
    hlsl::uint2 ChosenPixelCoords = s_defaultChosenPixelIdx;

    int ForcedLightIndex = -1;

    float ScaleIntensityGlobal = 1.0f;

    float ScaleIntensityDirect = 1.0f;
    float ScaleIntensityIndirect = 1.0f;

    float ScaleIntensityPunctual = 0.01f;
    float ScaleIntensityEnvMap = 1.0f;
    float ScaleIntensityEmission = 1.0f;

    float ScaleIntensityPoint = 0.1f;
    float ScaleIntensityDistant = 1.0f;
    float ScaleIntensitySpot = 1.0f;
    float ScalePointLightRadius = 0.0f;

    float ScaleF = 1.0f;
    float ScaleD = 1.0f;
    float ScaleG = 1.0f;

    float ScaleDiffuse = 1.0f;
    float ScaleSpecular = 1.0f;
    float ScaleReflect = 1.0f;
    float ScaleRefract = 1.0f;
    float ScaleClearcoat = 1.0f;

    bool FeatFlagQuickSwitchEnabled = false;
    FeatFlagsCore FeatFlagsCoreQS = static_cast<FeatFlagsCore>(0);
    FeatFlagsDbg FeatFlagsDebugQS = static_cast<FeatFlagsDbg>(0);
#endif
};

inline bool FeatFlagEnabledCore(
    const bool quickSwitchModeEnabled,
    const FeatFlagsCore& flags,
    const FeatFlagsCore& flagsQS,
    const FeatFlagsCore& flagValue)
{
    bool result = GetFeatFlagCore(flags, flagValue);

    const uint32_t idx = std::countr_zero(static_cast<uint32_t>(flagValue));
    if (quickSwitchModeEnabled && s_featFlagsIsQsCore[idx])
        result &= GetFeatFlagCore(flagsQS, flagValue);

    return result;
}

inline bool FeatFlagEnabledDebug(
    const bool quickSwitchModeEnabled,
    const FeatFlagsDbg& flags,
    const FeatFlagsDbg& flagsQS,
    const FeatFlagsDbg& flagValue)
{
    bool result = GetFeatFlagDebug(flags, flagValue);

    const uint32_t idx = std::countr_zero(static_cast<uint32_t>(flagValue));
    if (quickSwitchModeEnabled && s_featFlagsIsQsDbg[idx])
        result &= GetFeatFlagDebug(flagsQS, flagValue);

    return result;
}

struct PathTracerConfig
{
    uint32_t Seed = 1205;
    uint32_t SPP = 1;
    uint32_t MaxRayDepth = 8;
    uint32_t MaxShadowRayDepth = 8;
    uint32_t MaxFrameNumber = 0;
    uint32_t RussianRouletteMinBounces = 1;
    uint32_t DirectNumSamples = 1;

    uint32_t RestirConfidenceCap = 30;
    uint32_t RestirNumCandidates = 8;

    uint32_t TransientRenderNumSamples = 1000;
    uint32_t TransientRenderNumFrames = 60;

    uint32_t SprNumIterations = 6;

    int TransientLightIndex = -1;

    float TransientTimeSinceStart = 0.0f;
    float TransientPulseDuration = 1.0f;
    float TransientSpeedOfLight = 1.0f;
    float TransientRenderTotalTime = 10.0f;

    float FireFlyThreshold = 2.5f;
    float DofFocalDist = 0.1f;
    float DofLensRadius = 0.003f;

    float SprAlpha = 0.5f;

    bool TrueRandomSeedMode = false;
    bool PoissonReconstructionEnabled = false;
    bool DisplayGradientX = false;
    bool DisplayGradientY = false;

    FeatFlagsCore              FlagsCore           = s_defaultFeatFlagsCore;
    BxdfMode                   BxdfMode            = s_defaultBxdfMode;
    MicrofacetModelType        MicrofacetModelType = s_defaultMMType;

    PathTracingDebugInfo       DebugInfo        = {};

    bool FeatFlagEnabledCore(const FeatFlagsCore flags, const FeatFlagsCore flagValue) const
    {
        bool quickSwitchModeEnabled = false;
        FeatFlagsCore flagsQS = static_cast<FeatFlagsCore>(0);
#if CHERRY_DEBUG_FEATURES_ENABLED
        quickSwitchModeEnabled = DebugInfo.FeatFlagQuickSwitchEnabled;
        flagsQS = DebugInfo.FeatFlagsCoreQS;
#endif
        return ::FeatFlagEnabledCore(quickSwitchModeEnabled, flags, flagsQS, flagValue);
    }

    bool FeatFlagEnabledCore(const FeatFlagsCore flag) const
    {
        return FeatFlagEnabledCore(FlagsCore, flag);
    }

    bool FeatFlagEnabledDebug(const FeatFlagsDbg flags, const FeatFlagsDbg flag) const
    {
#if CHERRY_DEBUG_FEATURES_ENABLED
        return ::FeatFlagEnabledDebug(DebugInfo.FeatFlagQuickSwitchEnabled, flags, DebugInfo.FeatFlagsDebugQS, flag);
#endif
        return false;
    }

    bool FeatFlagEnabledDebug(const FeatFlagsDbg flag) const
    {
#if CHERRY_DEBUG_FEATURES_ENABLED
        return FeatFlagEnabledDebug(DebugInfo.FlagsDebug, flag);
#endif
        return false;
    }
};

struct ForwardConfig
{
    int TODO;
};

enum class RenderBackendMode : uint32_t
{
    ePathTracer,
    eForward,
    eCount
};

struct RenderBackendConfig
{
    XMFLOAT3 DirLightDirection = { -1, -1, -1 };
    XMFLOAT3 DirLightColor = { 1, 1, 1 };
    float DirLightIntensity = 1.0f;
    float DirLightCosAngularRadius = 0.00465f;
};

struct GreenhouseConfig
{
    RenderBackendMode RenderBackend = RenderBackendMode::ePathTracer;

    RenderBackendConfig RenderBackendConfig{};
    PathTracerConfig PathTracerConfig{};
    ForwardConfig ForwardConfig{};
};

#endif