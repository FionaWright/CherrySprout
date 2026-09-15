#ifndef H_GRADIENT_DOMAIN_STRUCTS_H
#define H_GRADIENT_DOMAIN_STRUCTS_H

#include "Utils/HlslGlue.h"
#include "Utils/Random.h"
#include "Utils/Math/ShadingFrame.h"

struct PathVertex
{
    hlsl::float3 Position;

    ShadingFrame SFrame;
    hlsl::float3 Wo;
    hlsl::float3 Wi;

    hlsl::uint IsDiffuse;
    hlsl::uint IsNextVertexDiffuse;
    float Eta;
    hlsl::float3 IndirectContribution;
};

#define PATH_MAX_VERTICES 16 // TODO

struct PathVertexList
{
    PathVertex Array[PATH_MAX_VERTICES];
    hlsl::uint NumVertices;
};

#endif