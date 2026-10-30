#ifndef H_PATH_TRACING_STRUCTS_H
#define H_PATH_TRACING_STRUCTS_H

#include "PathTracing/NEE/Structs.h"
#include "Utils/HlslGlue.h"

#include "Scene/Material.h"
#include "Utils/Math/ShadingFrame.h"

struct HitInfo
{
    Material Mat;

    hlsl::float3 Ng;
    hlsl::float3 Ng_ff;
    hlsl::float3 Ns_ff;
    ShadingFrame SFrame;

    hlsl::float3 Emission;

    hlsl::float2 AnisoDir;
    float AnisoStrength;

    hlsl::float2 UV;
    bool IsEntering;
    float RayT;

    hlsl::float3 HitPos;
    hlsl::uint PrimitiveCount;

    hlsl::float3 HitPosBiased;
    float TriangleArea;

    hlsl::uint InstanceIdx;
    hlsl::float3 p;
};

struct RngValuesPrimary
{
    hlsl::float2 Jitter;
    hlsl::float2 DoF;
};

struct RngValuesAncillary
{
    // === Batch 1 ===
    float AlphaTest;

    struct RngValuesNEE
    {
        float LSD;
        hlsl::float2 Light;
    } NEE[MAX_DIRECT_SAMPLES];

    struct RngValuesBxDF
    {
        hlsl::float3 Microfacet;
        hlsl::float2 LobeSelect;
    } BxDF;
    // ===============

    // === Batch 2 ===
    float RussianRoulette;
    // ===============

    // Note: ReSTIR generates other RNG values that aren't included here
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
    float RollingPathDistance;

    hlsl::float3 Lo;
    float LastBxdfPdf;

    hlsl::uint LastRayWasDiracDelta;
    hlsl::uint RaySegmentIdx;
    hlsl::uint IsPrimaryRay;
    float p;
};

#include "GradientDomain/Structs.h"

// Note: Unaligned, do not use in C++
struct PathSample
{
    hlsl::float3 Lo;
    PathVertexList VertexList;
    RngValuesPrimary RngPrimary;
};

struct ProbabilityDistributionSample
{
    float PDF;
    float CDF;
};

#endif