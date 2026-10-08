#include "Utils/SharedUtils.h"
#include "Utils/CBVs.h"

#include "Scene/Vertex.h"
#include "Scene/Material.h"
#include "Scene/InstanceData.h"

StructuredBuffer<InstanceData> gMegaBufferInstances : register(t0);
StructuredBuffer<Material> gMegaBufferMaterials : register(t1);
StructuredBuffer<Vertex> gMegaBufferVertex : register(t2);
StructuredBuffer<uint3> gMegaBufferIndex : register(t3);

StructuredBuffer<uint> gEmissiveInstanceToInstanceMap  : register(t4);

RWStructuredBuffer<float> gPDF  : register(u0);

float GetTriangleArea(InstanceData instance, Vertex v0, Vertex v1, Vertex v2)
{
    float3 p0 = mul(instance.M, float4(v0.Position,1)).xyz;
    float3 p1 = mul(instance.M, float4(v1.Position,1)).xyz;
    float3 p2 = mul(instance.M, float4(v2.Position,1)).xyz;

    float3 e1 = p1 - p0;
    float3 e2 = p2 - p0;

    return 0.5f * length(cross(e1, e2));
}

float GetTotalAreaLuminance(InstanceData instance)
{
    Material material = gMegaBufferMaterials[instance.MaterialIndex];
    float3 emissionMat = material.EmissiveColor * material.EmissiveStrength;

    uint primitiveOffset = instance.MegaBufferOffsetIndex / 3;

    float totalLuminance = 0.0f;

    [loop]
    for (uint i = 0; i < instance.MegaBufferCountIndex; i += 3)
    {
        uint3 tri = gMegaBufferIndex[primitiveOffset + i];

        Vertex v0 = gMegaBufferVertex[instance.MegaBufferOffsetVertex + tri.x];
        Vertex v1 = gMegaBufferVertex[instance.MegaBufferOffsetVertex + tri.y];
        Vertex v2 = gMegaBufferVertex[instance.MegaBufferOffsetVertex + tri.z];

        float triangleArea = GetTriangleArea(instance, v0, v1, v2);

        // TODO: Texture Sampling

        totalLuminance += Luminance(triangleArea * emissionMat);
    }

    return totalLuminance;
}

[numthreads(32,1,1)]
void CSMain(uint3 DTid : SV_DispatchThreadID)
{
    uint count, _;
    gEmissiveInstanceToInstanceMap.GetDimensions(count, _);

    uint emissiveInstanceIdx = DTid.x;

    if (emissiveInstanceIdx >= count)
        return;

    uint instanceIdx = gEmissiveInstanceToInstanceMap[emissiveInstanceIdx];
    InstanceData instance = gMegaBufferInstances[instanceIdx];

    float pdf = GetTotalAreaLuminance(instance);

    gPDF[emissiveInstanceIdx] = pdf;
}
