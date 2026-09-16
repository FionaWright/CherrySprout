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
    float PDF;
};

#define PATH_MAX_VERTICES 16 // TODO

struct PathVertexList
{
    PathVertexInfo Array[PATH_MAX_VERTICES];
    hlsl::uint NumVertices;
};

#define GDPT_VERTEX_DIFFUSE_THRESHOLD 0.001f // TODO: Make CBV option?

inline bool GetIsVertexDiffuse(const float roughness)
{
    return (roughness >= GDPT_VERTEX_DIFFUSE_THRESHOLD);
}

#endif