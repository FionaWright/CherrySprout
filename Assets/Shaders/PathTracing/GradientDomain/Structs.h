#ifndef H_GRADIENT_DOMAIN_STRUCTS_H
#define H_GRADIENT_DOMAIN_STRUCTS_H

#include "BxDFs/Structs.h"
#include "Utils/HlslGlue.h"
#include "Utils/Math/ShadingFrame.h"

#include "PathTracing/NEE/Structs.h"

enum class VertexType : hlsl::uint
{
    eUninitialized,
    eStandard,
    eEnvironment,
    eEmission,
};

struct PathVertexInfo
{
    VertexType Type;
    LobeTag Lobe;
    hlsl::float3 Position;

    hlsl::float3 Ng_ff;
    hlsl::float3 Wo;
    hlsl::float3 Wi;
    float Eta;
    float Roughness;

    hlsl::float3 IndirectContribution;
    hlsl::float3 DirectContribution;
    float PDF;

    LightSample LightSample; // TODO: Multiple NEE samples
    float ShadowFactor;
};

#define PATH_MAX_VERTICES 16 // TODO

struct PathVertexList
{
    PathVertexInfo Array[PATH_MAX_VERTICES];
    hlsl::uint NumVertices;
};

struct Gradients
{
    hlsl::float3 XForward;
    hlsl::float3 XBackward;
    hlsl::float3 YForward;
    hlsl::float3 YBackward;
};

enum class ReconnectionState : hlsl::uint
{
    eUnconnected,
    eSemiConnected,  // Connected but has different wo so needs BxDF evals
    eConnected,      // Can reuse Hit contribution
};

#endif