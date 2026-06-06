#ifndef H_CBVS_H
#define H_CBVS_H

#include "Utils/HlslGlue.h"
#include "PathTracing/Flags.h"

struct CbvPathTracingSettings
{
    hlsl::float4x4 InvP;
    hlsl::float4x4 InvV;

    hlsl::float3 CameraPositionWorld;
    hlsl::uint MaxRayDepth;

    hlsl::uint RussianRouletteMinBounces;
    hlsl::uint SPP;
    hlsl::uint FrameIdx;
    float pad;

    float DofFocalDist;
    float DofLensRadius;
    hlsl::float2 TexelSize;

    hlsl::float3 DirLightDirection;
    float DirLightCosAngularRadius;

    hlsl::float3 DirLightColor;
    float DirLightIntensity;

    float FireflyThreshold;
    hlsl::uint2 FrameDimensions;
};

#endif