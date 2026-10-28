#include "Utils/CBVs.h"
#include "Utils/SharedUtils.h"
#include "Utils/HlslUtils.hlsli"

TextureCube<float4> gSrc : register(t0);
RWTexture2DArray<float4> gDst : register(u0);
SamplerState gSamplerLinearClamp : register(s0);

ConstantBuffer<CbvMipMapCubemap> gCBV : register(b0);

float RadicalInverse_VdC(uint bits);
float2 RNG_Hammersley(uint i, uint N);
float3 ImportanceSampleGGX(float2 Xi, float3 N, float roughness);

#define SAMPLE_COUNT 1024u

[numthreads(8, 8, 6)]
void CSMain(uint3 DTid : SV_DispatchThreadID)
{
    uint2 coord = DTid.xy;
    uint face = DTid.z;

    if (any(coord >= gCBV.FaceResolution))
        return;

    float2 center = float2(coord) + 0.5f;
    float2 uv = center / float2(gCBV.FaceResolution);
    uv = RemapUtoS(uv);

    float3 N = CubemapCubeToSphere(face, uv);
    float3 R = N;
    float3 V = R;

    float totalWeight = 0.0;
    float3 prefilteredColor = float3(0.0, 0.0, 0.0);

    for (uint i = 0u; i < SAMPLE_COUNT; ++i)
    {
        float2 Xi = RNG_Hammersley(i, SAMPLE_COUNT);
        float3 H = ImportanceSampleGGX(Xi, N, gCBV.Roughness);
        float3 L = normalize(2.0 * dot(V, H) * H - V);

        float NdL = max(0.0f, dot(N, L));
        if (NdL > 0.0)
        {
            prefilteredColor += gSrc.Sample(gSamplerLinearClamp, L).rgb * NdL;
            totalWeight += NdL;
        }
    }

    prefilteredColor = prefilteredColor / max(1e-6f, totalWeight);
    gDst[DTid] = float4(prefilteredColor, 1.0);
}

float RadicalInverse_VdC(uint bits)
{
    bits = (bits << 16u) | (bits >> 16u);
    bits = ((bits & 0x55555555u) << 1u) | ((bits & 0xAAAAAAAAu) >> 1u);
    bits = ((bits & 0x33333333u) << 2u) | ((bits & 0xCCCCCCCCu) >> 2u);
    bits = ((bits & 0x0F0F0F0Fu) << 4u) | ((bits & 0xF0F0F0F0u) >> 4u);
    bits = ((bits & 0x00FF00FFu) << 8u) | ((bits & 0xFF00FF00u) >> 8u);
    return float(bits) * 2.3283064365386963e-10; // / 0x100000000
}

// Low discrepency psuedo random number generator
float2 RNG_Hammersley(uint i, uint N)
{
    return float2(float(i) / float(N), RadicalInverse_VdC(i));
}

float3 ImportanceSampleGGX(float2 Xi, float3 N, float roughness)
{
    float a = roughness * roughness;
    float a2 = a * a;

    float phi = 2.0 * PI * Xi.x;
    float cosTheta = sqrt((1.0 - Xi.y) / (1.0 + (a2 - 1.0) * Xi.y));
    float sinTheta = sqrt(1.0 - cosTheta * cosTheta);

    // from spherical coordinates to cartesian coordinates
    float3 H;
    H.x = cos(phi) * sinTheta;
    H.y = sin(phi) * sinTheta;
    H.z = cosTheta;

    // from tangent-space vector to world-space sample vector
    float3 up = abs(N.z) < 0.999 ? float3(0.0, 0.0, 1.0) : float3(1.0, 0.0, 0.0);
    float3 tangent = normalize(cross(up, N));
    float3 bitangent = cross(N, tangent);

    float3 sampleVec = tangent * H.x + bitangent * H.y + N * H.z;
    return normalize(sampleVec);
}