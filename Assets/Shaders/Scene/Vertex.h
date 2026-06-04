#ifndef H_VERTEX_H
#define H_VERTEX_H

#include "Utils/HlslGlue.h"

struct Vertex
{
    hlsl::float3 Position;
    hlsl::float3 Normal;
    hlsl::float2 UV;

#ifdef __cplusplus
    bool operator==(const Vertex& rhs) const
    {
        return Position.x == rhs.Position.x &&
                Position.y == rhs.Position.y &&
                Position.z == rhs.Position.z &&
                Normal.x == rhs.Normal.x &&
                Normal.y == rhs.Normal.y &&
                Normal.z == rhs.Normal.z &&
                UV.x == rhs.UV.x &&
                UV.y == rhs.UV.y;
    }
#endif
};

#endif