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

    hlsl::float3 Li;

    hlsl::float2 AnisoDir;
    float AnisoStrength;

    hlsl::float2 UV;
    bool IsEntering;
    float RayT;
};

struct PathState
{
    float LastBxdfPdf;
    hlsl::uint RaySegmentIdx;
    bool LastRayDiracDelta;
};

#endif