#ifndef H_GREENHOUSE_CONFIG_H
#define H_GREENHOUSE_CONFIG_H

#include "BxDFs/BxDFMode.h"
#include "PathTracing/Flags.h"
#include "PathTracing/Debug/OutputColor.h"

struct PathTracerConfig
{
    uint32_t SPP = 1;
    uint32_t MaxRayDepth = 8;
    uint32_t MaxShadowRayDepth = 1; // Transmission unsupported for shadow rays for now
    uint32_t MaxFrameNumber = 0;
    uint32_t RussianRouletteMinBounces = 1;

    float FireFlyThreshold = 2.5f;
    float DofFocalDist = 0.1f;
    float DofLensRadius = 0.003f;

    PathTracerFeatureFlags     FeatureFlags     = s_defaultFeatureFlags;
    PathTracerDebugFlags       DebugFlags       = s_defaultDebugFlags;
    DebugOutputIndex           DebugOutputIdx   = s_defaultOutputIndex;
    BxdfMode                   BxdfMode         = s_defaultBxdfMode;
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