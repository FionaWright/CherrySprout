#ifndef H_VERTEX_H
#define H_VERTEX_H

#include "Utils/HlslGlue.h"

struct Vertex
{
    hlsl::float3 Position;
    hlsl::float3 Normal;
    hlsl::float2 UV;
};

#endif