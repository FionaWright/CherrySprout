#ifndef H_CBVS_H
#define H_CBVS_H

#include "Utils/HlslGlue.h"

#include "../PathTracing/FeatFlags/Internal/Interpretations.h"

struct CbvPathTracingSettings
{
    hlsl::float4x4 InvP;

    hlsl::float4x4 InvV;

    hlsl::float3 CameraPositionWorld;
    hlsl::uint MaxRayDepth;

    hlsl::uint RussianRouletteMinBounces;
    hlsl::uint SPP;
    hlsl::uint FrameIdx;
    float p;

    float DofFocalDist;
    float DofLensRadius;
    hlsl::float2 TexelSize;

    hlsl::float3 DirLightDirection;
    float DirLightCosAngularRadius;

    hlsl::float3 DirLightColor;
    float DirLightIntensity;

    hlsl::uint2 FrameDimensions;
    float FireflyThreshold;
    hlsl::uint MaxShadowRayDepth;

    int TransientLightIdx;
    float TransientDistanceSinceStart;
    float TransientPulseDistance;
    hlsl::uint DirectNumSamples;

    hlsl::uint Seed;
    hlsl::uint LsdBasePunctuals;
    hlsl::uint LsdBaseEmissives;
    hlsl::uint LsdCount;

    float NearPlane;
    float FarPlane;
    hlsl::float2 JitterPerFrame;
};

struct CbvRestirSettings
{
    hlsl::uint GenConfidenceCap;
    hlsl::uint TemporalConfidenceCap;
    hlsl::uint GenCandidatesCount;
    hlsl::uint SpatialNumSamples;

    float TemporalNormalDivThreshold;
    float SpatialNormalDivThreshold;
    float TemporalDepthDivThreshold;
    float SpatialDepthDivThreshold;

    hlsl::uint SpatialConfidenceCap;
    float SpatialRadius;
    hlsl::uint TemporalEnabled; // TODO: Could be flags
    hlsl::uint SpatialEnabled;
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
    hlsl::float4x4 V, P, PrevV;
};

struct CbvColor
{
    hlsl::float4 Color;
};

enum class ForwardFlags : hlsl::uint
{
    eNone = 0,

    eEnvironmentMap = 1 << 0,
    ePunctualLights = 1 << 1,
    eDirectionalLight = 1 << 2,
    eShadowMaps = 1 << 3,
    eAmbient = 1 << 4,
};

inline bool ForwardFlagGet(const ForwardFlags flags, const ForwardFlags flag)
{
    return (hlsl::uint)flags & (hlsl::uint)flag;
}

inline void ForwardFlagSet(GLUE_INOUT(ForwardFlags) flags, const ForwardFlags flag, const bool state)
{
    hlsl::uint flagsU = (hlsl::uint)flags;

    if (state)
        flagsU |= ((hlsl::uint)flag);
    else if (ForwardFlagGet(flags, flag))
        flagsU &= ~((hlsl::uint)flag);

    flags = (ForwardFlags)flagsU;
}

struct CbvForward
{
    hlsl::float3 DirLightDir;
    hlsl::uint MaxCubemapMipMaps;

    hlsl::uint FrameIdx;
    hlsl::float3 CameraPosition;

    ForwardFlags Flags;
    hlsl::float3 p;
};

struct CbvMipMapCubemap
{
    hlsl::uint2 FaceResolution;
    float Roughness;
    float p;
};

struct CbvForward_PerInstance
{
    hlsl::float4x4 M, MTI;

    hlsl::uint MaterialIdx;
    float ScaleY;
    hlsl::float2 p;
};

struct CbvShadowMap_PerInstance
{
    hlsl::float4x4 M, V, P;

    hlsl::uint MaterialIdx;
    hlsl::float3 PointLightPosition;

    float FarPlane;
    hlsl::float3 p;
};

#define SHADOW_MAP_MAX_CASCADES 1 // TODO
struct CbvShadowMapSample
{
    hlsl::float4x4 ShadowMatrices[SHADOW_MAP_MAX_CASCADES];

    hlsl::float2 TexelSize;
    hlsl::uint CascadeCount;
    float BiasDepth;

    hlsl::float4 CascadeDistances;

    hlsl::uint PcfSampleCount;
    float PcfRadius;
    float NormalBias;
    float p;

#define POISSON_DISC_COUNT 8
    hlsl::float4 PoissonDisc[POISSON_DISC_COUNT];
};

struct CbvGBufferPrePass_PerInstance
{
    hlsl::uint InstanceIdx;
    hlsl::float3 p;
};

struct CbvLSD
{
    float EnvMapTotalLuminance;
    float PunctualTotalLuminance;
    float EmissiveTotalLuminance;
    hlsl::uint PunctualCount;

    hlsl::uint EmissiveCount;
    hlsl::float3 p;
};

struct CbvPathTracingDebugSettings
{
    hlsl::uint2 ChosenPixelCoords;
    hlsl::uint OutputColorIdx;
    hlsl::uint OutputColorRemapIdx;

    int ChosenRayDepth;
    float ScaleIntensityGlobal;

    float ScaleIntensityDirect;
    float ScaleIntensityIndirect;

    float ScaleIntensityPunctual;
    float ScaleIntensityEnvMap;
    float ScaleIntensityEmission;

    float ScaleIntensityPoint;
    float ScaleIntensityDistant;
    float ScaleIntensitySpot;
    float ScaleIntensityRect;
    float ScalePointLightRadius;

    float ScaleF;
    float ScaleD;
    float ScaleG;
    float ScaleDiffuse;

    float ScaleSpecular;
    float ScaleReflect;
    float ScaleRefract;
    int ForcedLightIndex;

    FeatFlagsCore FeatFlagsCoreQS;
    FeatFlagsDbg FeatFlagsDbgQS;
    float FixedMisNee;
    float p;
};

struct CbvSprJacobi
{
    hlsl::uint PrimalWidth;
    hlsl::uint PrimalHeight;
    float Alpha;
};

enum TextureConvertMode : hlsl::uint
{
    eNormalChannels2To3,
    eOgmSplit,
    eOrmSplit
};

struct CbvTextureConvert
{
    TextureConvertMode Mode;
    hlsl::float3 p;
};

struct CbvEmissivePdf
{
    hlsl::uint NumEmissiveInstances;
    float p[3];
};

struct CbvPostProcessing
{
    float ToneMapWhitePoint;
    float Exposure;
    hlsl::float2 p;
};

#endif