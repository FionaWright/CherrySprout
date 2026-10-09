#ifndef H_EVALUATE_EMISSIVE_H
#define H_EVALUATE_EMISSIVE_H

#if FEATURE_ENABLED_PP(NEE)

#include "PathTracing/HitInfo/ExtractUtils.hlsli"
#include "PathTracing/HitInfo/ApplyMaterialTextures.hlsli"

// TODO: Build PDF over emissive triangles instead of emissive instances

float3 SampleEmissiveBary(inout RngInfo rngInfo)
{
    float u = Rand01(rngInfo);
    float v = Rand01(rngInfo);

    if (u + v > 1.0)
    {
        u = 1.0 - u;
        v = 1.0 - v;
    }

    return float3(1.0 - u - v, u, v);
}

// Note: Ng, wo from perspective of the light
float EvaluateEmissivePdf(uint primitiveCount, float triangleArea, float3 Ng, float3 wo, float distance)
{
    float pdf_prim = 1.0f / (float)primitiveCount;

    float pdf_bary = 1.0f / triangleArea;

    float pdf_area = pdf_prim * pdf_bary;

    float NdL = abs(dot(Ng, wo));
    return PdfAreaToSolidAngle(pdf_area, NdL, distance);
}

LightSample SampleEmissive(inout RngInfo rngInfo, uint emissiveIdx, HitInfo hitInfo)
{
    uint instanceIdx = gEmissiveInstanceToInstanceMap[emissiveIdx];
    InstanceData instance = gMegaBufferInstanceData[instanceIdx];
    Material material = gMegaBufferMaterials[instance.MaterialIndex];

    uint primitiveCount = instance.MegaBufferCountIndex / 3;
    uint primitiveIdx = RandRangeUInt(rngInfo, 0, primitiveCount-1);
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

    float3 Ng = normalize( cross(p1 - p0, p2 - p0) );

    float3 destPos = p0 * bary.x + p1 * bary.y + p2 * bary.z;

    float3 emissionMat = material.EmissiveColor * material.EmissiveStrength;
    float3 emissionSample = SampleSceneTexture3(uv, material.TexIdxEmissive, 1);
    float3 emission = SRGB_to_LRGB(emissionSample) * emissionMat;

    float3 wi = normalize(destPos - hitInfo.HitPos);
    float dist = length(destPos - hitInfo.HitPos);

    float triangleArea = GetTriangleArea(p0, p1, p2);

    float pdf_angle = EvaluateEmissivePdf(primitiveCount, triangleArea, Ng, -wi, dist);

    LightSample lightSample;
    lightSample.Direction = wi;
    lightSample.Distance = dist;
    lightSample.Radiance = emission; // TODO: Units conversion?
    lightSample.PDF = pdf_angle;
    return lightSample;
}

LightSample EvaluateEmissive(uint emissiveIdx, HitInfo hitInfo, float3 wi)
{
    // TODO: Trace ray

    LightSample lightSample;
    lightSample.Direction = wi;
    lightSample.Distance = NAN;
    lightSample.Radiance = NAN;
    lightSample.PDF = NAN;
    return lightSample;
}

#else

float EvaluateEmissivePdf(uint primitiveCount, float triangleArea, float3 Ns, float3 wo, float distance) { return NAN; }

LightSample SampleEmissive(inout RngInfo rngInfo, uint emissiveIdx, HitInfo hitInfo) { return (LightSample)0; }

#endif

#endif