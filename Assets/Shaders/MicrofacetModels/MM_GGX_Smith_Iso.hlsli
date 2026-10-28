#ifndef H_GGX_SMITH_ISO_H
#define H_GGX_SMITH_ISO_H

#define MICROFACET_MODEL_CHOSEN

struct MicrofacetModel
{
#include "MicrofacetModels/IMicrofacetModel.hlsli"

    float m_alpha;
};

void MicrofacetModel::Init(float roughness, float3 V)
{
    m_alpha = RoughnessToAlpha(roughness);
}

uint MicrofacetModel::GetRngCount() { return 2; }

void MicrofacetModel::InitAniso(HitInfo _) { }

float MicrofacetModel::RoughnessToAlpha(float roughness)
{
    return roughness * roughness;
}

float3 MicrofacetModel::Sample(float3 rng)
{
    float a2 = m_alpha * m_alpha;

    float phi = 2.0 * PI * rng.x;
    float cosTheta = sqrt(max(0.0f, (1.0 - rng.y) / max(1e-6, 1.0 + (a2 - 1.0) * rng.y)));
    float sinTheta = sqrt(max(0.0f, 1.0 - cosTheta * cosTheta));

    float3 H_s = normalize(float3(sinTheta * cos(phi), sinTheta * sin(phi), cosTheta));
    return H_s;
}

float MicrofacetModel::D(float3 H)
{
    float NdH = H.z;

    if (NdH <= 0.0f || m_alpha <= 0.0f)
        return 0.0f;

    float a2 = m_alpha * m_alpha;

    float sx = -H.x / (NdH * m_alpha);
    float sy = -H.y / (NdH * m_alpha);
    float sl = 1.0f + sx * sx + sy * sy;
    float NdH4 = NdH * NdH * NdH * NdH;
    return 1.0f / ((sl * sl) * PI * a2 * NdH4);
}

float MicrofacetModel::G1(float3 W)
{
    float a2 = m_alpha * m_alpha;

    float NdW = W.z;

    float denom = NdW + sqrt(max(0.0f, a2 + (1-a2) * NdW * NdW));
    return saturate(2 * NdW / max(1e-6, denom));
}

float MicrofacetModel::G2(float3 L, float3 V)
{
    return G1(L) * G1(V);
}

float MicrofacetModel::PDF(float D, float3 H, float3 V)
{
    float NdH = H.z;

    if (NdH <= 0.0f)
        return 0.0f;

    return D * NdH;
}

#endif