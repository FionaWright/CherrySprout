#ifndef H_GRADIENT_DOMAIN_STRUCTS_H
#define H_GRADIENT_DOMAIN_STRUCTS_H

#include "Utils/HlslGlue.h"
#include "Utils/Random.h"

struct PathVertex // Include HitInfo instead of normal?
{
    hlsl::float3 Position;
    float PdfForward;

    hlsl::float3 Normal;
    float PdfReverse;

    hlsl::float3 Wo;
    hlsl::uint IsDelta;

    hlsl::float3 Wi;
    float p;
};

#define PATH_MAX_VERTICES 16 // TODO

struct PathSample
{
    PathVertex Vertices[PATH_MAX_VERTICES];
    hlsl::uint NumVertices;

    hlsl::float3 Lo;

    // TODO: Needed?
    hlsl::float3 Origin;
    hlsl::float3 Direction;
    RngInfo RngInfo;
    hlsl::uint2 PixelCoord;
};

struct GradientSample
{
    hlsl::float3 Primal;
    hlsl::float3 GX;
    hlsl::float3 GY;
};

#endif