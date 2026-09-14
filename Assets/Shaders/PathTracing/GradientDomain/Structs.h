#ifndef H_GRADIENT_DOMAIN_STRUCTS_H
#define H_GRADIENT_DOMAIN_STRUCTS_H

#include "Utils/HlslGlue.h"
#include "Utils/Random.h"

struct PathVertex // Include HitInfo instead of normal?
{
    hlsl::float3 Position;
    hlsl::float3 Normal;

    hlsl::float3 Wo;
    hlsl::float3 Wi;

    hlsl::uint IsDiffuse;
};

#define PATH_MAX_VERTICES 16 // TODO

struct PathVertexList
{
    PathVertex Vertices[PATH_MAX_VERTICES];
    hlsl::uint NumVertices;
};

#endif