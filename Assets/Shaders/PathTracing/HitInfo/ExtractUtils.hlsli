#ifndef H_EXTRACT_UTILS_H
#define H_EXTRACT_UTILS_H

#include "PathTracing/Structs.h"

void ExtractVertices(RayQuery<RAY_FLAGS> q, InstanceData instance, out Vertex v0, out Vertex v1, out Vertex v2)
{
    uint primitiveIdx = q.CommittedPrimitiveIndex();
    uint primitiveOffset = instance.MegaBufferOffsetIndex / 3;
    uint3 tri = gMegaBufferIndex[primitiveOffset + primitiveIdx];

    v0 = gMegaBufferVertex[instance.MegaBufferOffsetVertex + tri.x];
    v1 = gMegaBufferVertex[instance.MegaBufferOffsetVertex + tri.y];
    v2 = gMegaBufferVertex[instance.MegaBufferOffsetVertex + tri.z];
}

float3 ExtractBarycentrics(RayQuery<RAY_FLAGS> q)
{
    float2 barycentrics = q.CommittedTriangleBarycentrics();
    DBG_OUTPUT2(barycentrics, Hit_Barycentrics);

    return float3(1 - barycentrics.x - barycentrics.y, barycentrics.x, barycentrics.y);
}

float2 ExtractUV(RayQuery<RAY_FLAGS> q, InstanceData instance)
{
    Vertex v0, v1, v2;
    ExtractVertices(q, instance, v0, v1, v2);

    float3 bary = ExtractBarycentrics(q);

    float2 uv = v0.UV * bary.x + v1.UV * bary.y + v2.UV * bary.z;
    uv.y = 1 - uv.y;
    return uv;
}

void ExtractInterpolatedAttributes(RayQuery<RAY_FLAGS> q, InstanceData instance, out float3 Ns, out float3 Ng, out float2 uv)
{
    Vertex v0, v1, v2;
    ExtractVertices(q, instance, v0, v1, v2);

    float3 bary = ExtractBarycentrics(q);

    Ns = v0.Normal * bary.x + v1.Normal * bary.y + v2.Normal * bary.z;
    Ns = normalize(mul((float3x3)instance.MTI, Ns));

    float3 p0 = mul(instance.M, float4(v0.Position,1)).xyz;
    float3 p1 = mul(instance.M, float4(v1.Position,1)).xyz;
    float3 p2 = mul(instance.M, float4(v2.Position,1)).xyz;
    Ng = normalize( cross(p1 - p0, p2 - p0) );

    uv = v0.UV * bary.x + v1.UV * bary.y + v2.UV * bary.z;
    uv.y = 1 - uv.y;
}

#endif