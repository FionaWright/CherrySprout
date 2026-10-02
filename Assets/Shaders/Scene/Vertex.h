#ifndef H_VERTEX_H
#define H_VERTEX_H

#include "Utils/HlslGlue.h"

struct Vertex
{
    hlsl::float3 Position;
    hlsl::float3 Normal;
    hlsl::float2 UV;

#ifdef __cplusplus
    Vertex()
    {
        Position = hlsl::float3(0, 0, 0);
        Normal = hlsl::float3(0, 0, 0);
        UV = hlsl::float2(0, 0);
    }

    Vertex(const hlsl::float3 position, const hlsl::float3 normal, const hlsl::float2 uv)
    {
        Position = position;
        Normal = normal;
        UV = uv;
    }

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