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
    FeatFlagsDBG FlagsDebug = s_defaultFeatFlagsDBG;
    DebugOutputIndex OutputColorIdx = s_defaultOutputIndex;
    DebugOutputColorRemap OutputColorRemap = s_defaultOutputColorRemap;
    int ChosenRayDepth = -1;
    hlsl::uint2 ChosenPixelCoords = s_defaultChosenPixelIdx;

    int ForcedLightIndex = -1;

    float ScaleIntensityGlobal = 1.0f;

    float ScaleIntensityDirect = 1.0f;
    float ScaleIntensityIndirect = 1.0f;

    float ScaleIntensityPunctual = 1.0f;
    float ScaleIntensityEnvMap = 1.0f;
    float ScaleIntensityEmission = 1.0f;

    float ScaleIntensityPoint = 1.0f;
    float ScaleIntensityDistant = 1.0f;
    float ScaleIntensitySpot = 1.0f;
    float ScaleIntensityRect = 1.0f;
    float ScalePointLightRadius = 0.0f;

    float ScaleF = 1.0f;
    float ScaleD = 1.0f;
    float ScaleG = 1.0f;

    float ScaleDiffuse = 1.0f;
    float ScaleSpecular = 1.0f;
    float ScaleReflect = 1.0f;
    float ScaleRefract = 1.0f;
    float ScaleClearcoat = 1.0f;

    float FixedMisNee = 1.0f;

    bool FeatFlagQuickSwitchEnabled = false;
    FeatFlagsCORE FeatFlagsCoreQS = static_cast<FeatFlagsCORE>(0);
    FeatFlagsADV FeatFlagsAdvQS = static_cast<FeatFlagsADV>(0);
    FeatFlagsDBG FeatFlagsDebugQS = static_cast<FeatFlagsDBG>(0);
#endif
};

template <typename TFeatFlags>
bool FeatFlagEnabled(
    const bool quickSwitchModeEnabled,
    const TFeatFlags& flags,
    const TFeatFlags& flagsQS,
    const TFeatFlags& flagValue)
{
    bool result = GetFeatFlag(flags, flagValue);

    const uint32_t idx = std::countr_zero(static_cast<uint32_t>(flagValue));
    if (quickSwitchModeEnabled && s_featFlagsIsQsCORE[idx])
        result &= GetFeatFlag(flagsQS, flagValue);

    return result;
}

struct PathTracerConfig
{
    uint32_t Seed = 1205;
    uint32_t SPP = 1;
    uint32_t MaxRayDepth = 4;
    uint32_t MaxShadowRayDepth = 8;
    uint32_t MaxFrameCount = 0;
    uint32_t RussianRouletteMinBounces = 1;
    uint32_t DirectNumSamples = 1;

    uint32_t RestirGenConfidenceCap = 30; // TODO: Unused?
    uint32_t RestirTemporalConfidenceCap = 30;
    uint32_t RestirSpatialConfidenceCap = 30;
    uint32_t RestirGenCandidatesCount = 8;
    uint32_t RestirSpatialNumSamples = 4;

    uint32_t TransientRenderNumSamples = 1000;
    uint32_t TransientRenderNumFrames = 60;

    uint32_t SprNumIterations = 200;

    int TransientLightIndex = -1;

    float TransientTimeSinceStart = 0.0f;
    float TransientPulseDuration = 1.0f;
    float TransientSpeedOfLight = 1.0f;
    float TransientRenderTotalTime = 10.0f;

    float FireFlyThreshold = 2.5f;
    float DofFocalDist = 0.1f;
    float DofLensRadius = 0.003f;

    float SprAlpha = 0.0f;

    float RestirTemporalNormalDivThreshold = 0.9f;
    float RestirSpatialNormalDivThreshold = 0.9f;
    float RestirTemporalDepthDivThreshold = 5.0f;
    float RestirSpatialDepthDivThreshold = 5.0f;
    float RestirSpatialRadius = 30.0f;

    bool TrueRandomSeedMode = false;
    bool PoissonReconstructionEnabled = false;
    bool ScreenSpaceGradients = false;
    bool DisplayGradientX = false;
    bool DisplayGradientY = false;

    bool RestirTemporalEnabled = false;
    bool RestirSpatialEnabled = false;

    FeatFlagsCORE              FlagsCore           = s_defaultFeatFlagsCORE;
    FeatFlagsADV               FlagsAdv            = s_defaultFeatFlagsADV;
    BxdfMode                   BxdfMode            = s_defaultBxdfMode;
    MicrofacetModelType        MicrofacetModelType = s_defaultMMType;

    PathTracingDebugInfo       DebugInfo        = {};

    [[nodiscard]] bool FeatFlagEnabledCore(const FeatFlagsCORE flags, const FeatFlagsCORE flagValue) const
    {
        bool quickSwitchModeEnabled = false;
        auto flagsQS = static_cast<FeatFlagsCORE>(0);
#if CHERRY_DEBUG_FEATURES_ENABLED
        quickSwitchModeEnabled = DebugInfo.FeatFlagQuickSwitchEnabled;
        flagsQS = DebugInfo.FeatFlagsCoreQS;
#endif
        return ::FeatFlagEnabled(quickSwitchModeEnabled, flags, flagsQS, flagValue);
    }

    [[nodiscard]] bool FeatFlagEnabledCore(const FeatFlagsCORE flag) const
    {
        return FeatFlagEnabledCore(FlagsCore, flag);
    }

    [[nodiscard]] bool FeatFlagEnabledAdv(const FeatFlagsADV flags, const FeatFlagsADV flagValue) const
    {
        bool quickSwitchModeEnabled = false;
        auto flagsQS = static_cast<FeatFlagsADV>(0);
#if CHERRY_DEBUG_FEATURES_ENABLED
        quickSwitchModeEnabled = DebugInfo.FeatFlagQuickSwitchEnabled;
        flagsQS = DebugInfo.FeatFlagsAdvQS;
#endif
        return ::FeatFlagEnabled(quickSwitchModeEnabled, flags, flagsQS, flagValue);
    }

    [[nodiscard]] bool FeatFlagEnabledAdv(const FeatFlagsADV flag) const
    {
        return FeatFlagEnabledAdv(FlagsAdv, flag);
    }

    [[nodiscard]] bool FeatFlagEnabledDebug(const FeatFlagsDBG flags, const FeatFlagsDBG flag) const
    {
#if CHERRY_DEBUG_FEATURES_ENABLED
        return ::FeatFlagEnabled(DebugInfo.FeatFlagQuickSwitchEnabled, flags, DebugInfo.FeatFlagsDebugQS, flag);
#endif
        return false;
    }

    [[nodiscard]] bool FeatFlagEnabledDebug(const FeatFlagsDBG flag) const
    {
#if CHERRY_DEBUG_FEATURES_ENABLED
        return FeatFlagEnabledDebug(DebugInfo.FlagsDebug, flag);
#endif
        return false;
    }
};

struct RasterConfig
{
    uint32_t SmCascadeCount = 1;
    uint32_t SmPcfCount = 16;

    int BloomRadius = 5;
    uint32_t BloomDownsampleCount = 4;
    float BloomLumThreshold = 3.0f;

    float SmWidth = 100.0f;
    float SmHeight = 100.0f;
    float SmNear = 0.01f;
    float SmFar = 100.0f;
    float SmDirDist = 50.0f;
    float SmPcfRadius = 1.5f;
    float SmBiasDepthDistant = 0.000001f;
    float SmBiasDepthPoint = 0.001f;
    float SmBiasNormalDistant = 0.01f;
    float SmBiasNormalPoint = 0.001f;

    bool FlagEnabledEnvironmentMap = true;
    bool FlagEnabledPunctualLights = true;
    bool FlagEnabledDirectionalLight = true;
    bool FlagEnabledShadowMaps = true;
    bool FlagEnabledAmbient = true;
};

enum class RenderBackendMode : uint32_t
{
    ePathTracer,
    eForward,
    eDeferred,
    eCount
};

struct RenderBackendConfig
{
    XMFLOAT3 DirLightDirection = { -1, -0.7, -0.7 };
    XMFLOAT3 DirLightColor = { 1, 1, 1 };
    float DirLightIntensity = 1.0f;
    float DirLightCosAngularRadius = 0.00465f;

    float PostProcessExposure = 1.0f;
    float PostProcessWhitePoint = 1.0f;
};

struct GreenhouseConfig
{
    RenderBackendMode RenderBackend = RenderBackendMode::ePathTracer;

    bool AnimationEnabled = true;
    AnimationType AnimType = AnimationType::eLBS;

    RenderBackendConfig RenderBackendConfig{};
    PathTracerConfig PathTracerConfig{};
    RasterConfig RasterConfig{};
};

#endif