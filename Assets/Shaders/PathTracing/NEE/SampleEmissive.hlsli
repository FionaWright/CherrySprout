#ifndef H_EVALUATE_EMISSIVE_H
#define H_EVALUATE_EMISSIVE_H

#if FEAT_CORE_D(NEE)

#include "PathTracing/HitInfo/ExtractUtils.hlsli"
#include "PathTracing/HitInfo/ApplyMaterialTextures.hlsli"

// TODO: Build PDF over emissive triangles instead of emissive instances

// TODO: Option to disable NEE when ray segment idx == max depth

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

    float pdf_bary = 1.0f / max(1e-6f, triangleArea);

    float pdf_area = pdf_prim * pdf_bary;

    DBG_OUTPUT1(pdf_prim, NEE_PrimPdf);
    DBG_OUTPUT1(pdf_bary, NEE_BaryPdf);
    DBG_OUTPUT1(triangleArea, NEE_TriangleArea);

    float NdL = abs(dot(Ng, wo));
    return PdfAreaToSolidAngle(pdf_area, NdL, distance);
}

LightSample SampleEmissive(inout RngInfo rngInfo, uint emissiveIdx, HitInfo hitInfo, out LightEval lightEval)
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

    lightEval.Radiance = emission;
    lightEval.PDF = EvaluateEmissivePdf(primitiveCount, triangleArea, Ng, -wi, dist);

    LightSample lightSample = (LightSample)0;
    lightSample.Direction = wi;
    lightSample.Distance = dist;
    lightSample.UV = uv;
    lightSample.SubIndex = primitiveIdx;
    lightSample.IsDelta = false;
    return lightSample;
}

LightEval EvaluateEmissive(uint emissiveIdx, HitInfo hitInfo, LightSample lightSample)
{
    uint instanceIdx = gEmissiveInstanceToInstanceMap[emissiveIdx];
    InstanceData instance = gMegaBufferInstanceData[instanceIdx];
    Material material = gMegaBufferMaterials[instance.MaterialIndex];

    uint primitiveCount = instance.MegaBufferCountIndex / 3;
    uint primitiveOffset = instance.MegaBufferOffsetIndex / 3;
    uint3 tri = gMegaBufferIndex[primitiveOffset + lightSample.SubIndex];

    Vertex v0 = gMegaBufferVertex[instance.MegaBufferOffsetVertex + tri.x];
    Vertex v1 = gMegaBufferVertex[instance.MegaBufferOffsetVertex + tri.y];
    Vertex v2 = gMegaBufferVertex[instance.MegaBufferOffsetVertex + tri.z];

    float3 p0 = mul(instance.M, float4(v0.Position,1)).xyz;
    float3 p1 = mul(instance.M, float4(v1.Position,1)).xyz;
    float3 p2 = mul(instance.M, float4(v2.Position,1)).xyz;

    float3 Ng = normalize( cross(p1 - p0, p2 - p0) );

    float3 emissionMat = material.EmissiveColor * material.EmissiveStrength;
    float3 emissionSample = SampleSceneTexture3(lightSample.UV, material.TexIdxEmissive, 1);
    float3 emission = SRGB_to_LRGB(emissionSample) * emissionMat;

    float triangleArea = GetTriangleArea(p0, p1, p2);

    LightEval lightEval;
    lightEval.Radiance = emission;
    lightEval.PDF = EvaluateEmissivePdf(primitiveCount, triangleArea, Ng, -lightSample.Direction, lightSample.Distance);
    return lightEval;
}

#else

float EvaluateEmissivePdf(uint primitiveCount, float triangleArea, float3 Ns, float3 wo, float distance) { return NAN; }

LightSample SampleEmissive(inout RngInfo rngInfo, uint emissiveIdx, HitInfo hitInfo) { return (LightSample)0; }

LightEval EvaluateEmissive(uint emissiveIdx, HitInfo hitInfo, LightSample lightSample) { return (LightEval)0; }

#endif

#endif