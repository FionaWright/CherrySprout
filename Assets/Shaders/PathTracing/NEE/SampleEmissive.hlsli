#ifndef H_EVALUATE_EMISSIVE_H
#define H_EVALUATE_EMISSIVE_H

#include "PathTracing/HitInfo/ExtractUtils.hlsli"
#include "PathTracing/HitInfo/ApplyMaterialTextures.hlsli"

uint SampleEmissivePrimitive(inout RngInfo rngInfo, InstanceData instance) // TODO: PDF?
{
    uint primitiveCount = instance.MegaBufferCountIndex / 3;
    return RandRangeUInt(rngInfo, 0, primitiveCount-1);
}

uint3 SampleEmissiveBary(inout RngInfo rngInfo) // TODO: PDF?
{
    float baryX = Rand01(rngInfo);
    float baryY = Rand01(rngInfo);
    return float3(1 - baryX - baryY, baryX, baryY);
}

LightSample SampleEmissive(inout RngInfo rngInfo, uint emissiveIdx, float3 sourcePos)
{
    uint instanceIdx = gEmissiveInstanceToInstanceMap[emissiveIdx];
    InstanceData instance = gMegaBufferInstanceData[instanceIdx];

    uint primitiveIdx = SampleEmissivePrimitive(rngInfo, instance);
    float3 bary = SampleEmissiveBary(rngInfo);

    uint primitiveOffset = instance.MegaBufferOffsetIndex / 3;
    uint3 tri = gMegaBufferIndex[primitiveOffset + primitiveIdx];

    Vertex v0 = gMegaBufferVertex[instance.MegaBufferOffsetVertex + tri.x];
    Vertex v1 = gMegaBufferVertex[instance.MegaBufferOffsetVertex + tri.y];
    Vertex v2 = gMegaBufferVertex[instance.MegaBufferOffsetVertex + tri.z];

    float3 p0 = mul(instance.M, float4(v0.Position,1)).xyz;
    float3 p1 = mul(instance.M, float4(v1.Position,1)).xyz;
    float3 p2 = mul(instance.M, float4(v2.Position,1)).xyz;

    float2 uv = v0.UV * bary.x + v1.UV * bary.y + v2.UV * bary.z;
    uv.y = 1 - uv.y;

    float3 destPos = v0.Position * bary.x + v1.Position * bary.y + v2.Position * bary.z;

    //float triangleArea = GetTriangleArea(instance, v0, v1, v2);

    Material material = gMegaBufferMaterials[instance.MaterialIndex];

    float3 emissionMat = material.EmissiveColor * material.EmissiveStrength; // TODO: Assert emissionMat > 0
    float3 emissionSample = SampleSceneTexture3(uv, material.TexIdxEmissive, 1);
    float3 emission = SRGB_to_LRGB(emissionSample) * emissionMat;

    LightSample lightSample;
    lightSample.Direction = normalize(destPos - sourcePos);
    lightSample.Distance = length(destPos - sourcePos);
    lightSample.Radiance = emission; // TODO: Units conversion?
    lightSample.PDF = 1.0f;
    return lightSample;
}

#endif