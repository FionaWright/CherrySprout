#ifndef H_GRADIENT_DOMAIN_STRUCTS_H
#define H_GRADIENT_DOMAIN_STRUCTS_H

#include "Utils/HlslGlue.h"
#include "Utils/Random.h"
#include "Utils/Math/ShadingFrame.h"

enum class VertexType : hlsl::uint
{
    eUninitialized,
    eGlossy,
    eDiffuse,
    eEnvironment,
};

struct PathVertexInfo
{
    VertexType Type;
    hlsl::float3 Position;

    ShadingFrame SFrame;
    hlsl::float3 Wo;
    hlsl::float3 Wi;
    float Eta;

    hlsl::float3 IndirectContribution;
    hlsl::float3 DirectContribution;
    float PDF_Bxdf;
    float PDF; // BXDF + NEE

    hlsl::uint NeeLightIdx;
    hlsl::float3 NeeLightDirection;
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

#define GDPT_VERTEX_DIFFUSE_THRESHOLD 0.001f // TODO: Make CBV option?

inline bool GetIsVertexDiffuse(const float roughness)
{
    return (roughness >= GDPT_VERTEX_DIFFUSE_THRESHOLD);
}

enum class ReconnectionState : hlsl::uint
{
    eUnconnected,
    eSemiConnected,  // Connected but has different wo so needs BxDF evals
    eConnected,      // Can reuse Hit contribution
};

#endif