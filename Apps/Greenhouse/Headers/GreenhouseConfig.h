#ifndef H_GREENHOUSE_CONFIG_H
#define H_GREENHOUSE_CONFIG_H

#include "PathTracer.h"
#include "BxDFs/BxDFMode.h"
#include "PathTracing/Debug/OutputColor.h"
#include "PathTracing/Debug/OutputColorRemap.h"
#include "PathTracing/Flags/MethodsCpp.h"

constexpr hlsl::uint2 s_defaultChosenPixelIdx = hlsl::uint2(300, 300);

struct PathTracingDebugInfo
{
#if CHERRY_DEBUG_FEATURES_ENABLED
    PathTracerDebugFlags Flags = s_defaultDebugFlags;
    DebugOutputIndex OutputColorIdx = s_defaultOutputIndex;
    DebugOutputColorRemap OutputColorRemap = s_defaultOutputColorRemap;
    int ChosenRayDepth = -1;
    hlsl::uint2 ChosenPixelCoords = s_defaultChosenPixelIdx;

    int ForcedLightIndex = -1;

    float ScaleIntensityGlobal = 1.0f;
    float ScaleIntensityPunctual = 0.01f;
    float ScaleIntensityPoint = 0.1f;
    float ScaleIntensityDistant = 1.0f;
    float ScaleIntensitySpot = 1.0f;
    float ScaleIntensityEnvMap = 1.0f;
    float ScalePointLightRadius = 0.0f;
    float ScaleF = 1.0f;
    float ScaleD = 1.0f;
    float ScaleG = 1.0f;
    float ScaleDiffuse = 1.0f;
    float ScaleSpecular = 1.0f;
    float ScaleReflect = 1.0f;
    float ScaleRefract = 1.0f;
    float ScaleClearcoat = 1.0f;
#endif
};

struct PathTracerConfig
{
    uint32_t SPP = 1;
    uint32_t MaxRayDepth = 8;
    uint32_t MaxShadowRayDepth = 1; // Transmission unsupported for shadow rays for now
    uint32_t MaxFrameNumber = 0;
    uint32_t RussianRouletteMinBounces = 1;
    uint32_t DirectNumSamples = 1;

    float FireFlyThreshold = 2.5f;
    float DofFocalDist = 0.1f;
    float DofLensRadius = 0.003f;

    int TransientLightIndex = -1;
    float TransientTimeSinceStart = 0.0f;
    float TransientPulseDuration = 1.0f;
    float TransientSpeedOfLight = 1.0f;

    uint32_t TransientRenderNumSamples = 1000;
    uint32_t TransientRenderNumFrames = 60;
    float TransientRenderTotalTime = 10.0f;

    uint32_t RestirConfidenceCap = 30;
    uint32_t RestirNumCandidates = 8;

    PathTracerFeatureFlags     FeatureFlags     = s_defaultFeatureFlags;
    BxdfMode                   BxdfMode         = s_defaultBxdfMode;

    PathTracingDebugInfo       DebugInfo        = {};
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