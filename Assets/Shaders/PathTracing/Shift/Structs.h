#ifndef H_SHIFT_STRUCTS_H
#define H_SHIFT_STRUCTS_H

#include "BxDFs/Structs.h"
#include "Utils/HlslGlue.h"

#include "PathTracing/Structs.h"
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

    RngValuesAncillary RNG;
};

struct PathVertexList
{
    PathVertexInfo Array[MAX_RAY_DEPTH];
    hlsl::uint NumVertices;
};

inline PathVertexList CreatePathVertexList()
{
    PathVertexList vertexList;
    vertexList.NumVertices = 0;
    return vertexList;
}

#endif