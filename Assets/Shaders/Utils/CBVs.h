#ifndef H_CBVS_H
#define H_CBVS_H

#include "Utils/HlslGlue.h"
#include "PathTracing/Flags.h"
#include "Raster/RasterOutputMode.h"

struct CbvPathTracingSettings
{
    hlsl::float4x4 InvP;
    hlsl::float4x4 InvV;

    hlsl::float3 CameraPositionWorld;
    hlsl::uint MaxRayDepth;

    hlsl::uint RussianRouletteMinBounces;
    hlsl::uint SPP;
    hlsl::uint FrameIdx;
    hlsl::uint IsMaxFramesReached;

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

struct CbvPanoToEA
{
    hlsl::uint2 OutputDimensions;
    hlsl::uint2 InputDimensions;

    float Rotation;
    float p[3];
};

struct CbvPanoToCM
{
    hlsl::uint OutputWidth;
    hlsl::uint2 InputDimensions;
    float Rotation;
};

struct CbvMatrices_MVP
{
    hlsl::float4x4 M, MTI, V, P;
};

struct CbvMatrices_M
{
    hlsl::float4x4 M, MTI;
};

struct CbvMatrices_VP
{
    hlsl::float4x4 V, P;
};

struct CbvForward
{
    hlsl::float3 DirLightDir;
    hlsl::uint MaxCubemapMipMaps;

    RasterOutputMode OutputMode;
    hlsl::float3 _;
};

#endif