#ifndef H_CBVS_H
#define H_CBVS_H

#include "Utils/HlslGlue.h"

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

    hlsl::uint MaxShadowRayDepth;
    int TransientLightIdx;
    float TransientDistanceSinceStart;
    float TransientPulseDistance;

    hlsl::uint RestirConfidenceCap;
    hlsl::uint RestirNumCandidates;
    hlsl::uint2 p;
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

struct CbvGammaCorrect
{
    hlsl::uint2 Dimensions;
    hlsl::uint IsToSrgb;
    float p;
};

struct CbvMatrices_MVP
{
    hlsl::float4x4 M, MTI, V, P;
};

struct CbvMatrices_MVP_Lean
{
    hlsl::float4x4 M, V, P;
};

struct CbvMatrices_M
{
    hlsl::float4x4 M, MTI;
};

struct CbvMatrices_VP
{
    hlsl::float4x4 V, P;
};

struct CbvColor
{
    hlsl::float4 Color;
};

struct CbvForward
{
    hlsl::float3 DirLightDir;
    hlsl::uint MaxCubemapMipMaps;

    hlsl::uint OutputMode;
    hlsl::float3 _;
};

struct CbvGBufferPrePass_PerInstance
{
    hlsl::uint InstanceIdx;
    hlsl::float3 p;
};

struct CbvTotalLuminances
{
    float EnvMapTotalLuminance;
    float PunctualTotalLuminance;
};

struct CbvPathTracingDebugSettings
{
    hlsl::uint2 ChosenPixelCoords;
    hlsl::uint OutputColorIdx;
    hlsl::uint OutputColorRemapIdx;

    int ChosenRayDepth;
    float ScaleIntensityGlobal;
    float ScaleIntensityPunctual;
    float ScaleIntensityPoint;

    float ScaleIntensityDistant;
    float ScaleIntensitySpot;
    float ScaleIntensityEnvMap;
    float ScalePointLightRadius;

    float ScaleF;
    float ScaleD;
    float ScaleG;
    float ScaleDiffuse;

    float ScaleSpecular;
    float ScaleReflect;
    float ScaleRefract;
    float p;
};

#endif