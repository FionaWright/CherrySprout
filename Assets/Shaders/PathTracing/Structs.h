#ifndef H_PATH_TRACING_STRUCTS_H
#define H_PATH_TRACING_STRUCTS_H

#include "Utils/HlslGlue.h"

#include "Scene/Material.h"
#include "Utils/Math/ShadingFrame.h"

struct HitInfo
{
    Material Mat;

    hlsl::float3 Ng_ff;
    hlsl::float3 Ns_ff;
    ShadingFrame SFrame;

    hlsl::float3 Emission;

    hlsl::float2 AnisoDir;
    float AnisoStrength;

    hlsl::float2 UV;
    bool IsEntering;
    float RayT;
};

#ifdef __cplusplus
struct RayDesc
{
    hlsl::float3 Origin;
    float  TMin;

    hlsl::float3 Direction;
    float  TMax;
};
#endif

struct PathState
{
    RayDesc Desc;

    hlsl::float3 Beta;
    float LastBxdfPdf;

    hlsl::float3 Lo;
    hlsl::uint RaySegmentIdx;

    hlsl::float3 Gradient;
    float RollingPathDistance;

    hlsl::uint LastRayWasDiracDelta;
    hlsl::float3 p;
};

struct ProbabilityDistributionSample
{
    float PDF;
    float CDF;
};

struct LightSample
{
    hlsl::float3 Direction;
    float Distance;

    hlsl::float3 Radiance;
    hlsl::uint Index;

    bool IsDelta;
    float PDF;
};

#endif