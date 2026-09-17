#ifndef H_GGX_SMITH_ISO_H
#define H_GGX_SMITH_ISO_H

#define MICROFACET_MODEL_CHOSEN

struct MicrofacetModel
{
#include "MicrofacetModels/IMicrofacetModel.hlsli"

    float m_alpha;
};

void MicrofacetModel::Init(float roughness, RngInfo rngInfo, float3 V)
{
    m_alpha = RoughnessToAlpha(roughness);
}

void MicrofacetModel::Init(float roughness, float3 V)
{
    m_alpha = RoughnessToAlpha(roughness);
}

void MicrofacetModel::InitAniso(HitInfo _) { }

float MicrofacetModel::RoughnessToAlpha(float roughness)
{
    return roughness * roughness;
}

float3 MicrofacetModel::Sample(float u1, float u2)
{
    float a2 = m_alpha * m_alpha;

    float phi = 2.0 * PI * u1;
    float cosTheta = sqrt(max(0.0f, (1.0 - u2) / max(1e-6, 1.0 + (a2 - 1.0) * u2)));
    float sinTheta = sqrt(max(0.0f, 1.0 - cosTheta * cosTheta));

    float3 H_s = normalize(float3(sinTheta * cos(phi), sinTheta * sin(phi), cosTheta));
    return H_s;
}

float MicrofacetModel::D(float3 H)
{
    float a2 = m_alpha * m_alpha;
    float NdH = H.z;

    float denominator = (NdH * NdH * (a2 - 1.0f) + 1.0f);
    return a2 / max(1e-6, PI * denominator * denominator);
}

float MicrofacetModel::G1(float NdW)
{
    float a2 = m_alpha * m_alpha;

    float denom = NdW + sqrt(max(0.0f, a2 + (1-a2) * NdW * NdW));
    return saturate(2 * NdW / max(1e-6, denom));
}

float MicrofacetModel::G2(float NdL, float NdV)
{
    return G1(NdL) * G1(NdV);
}

float MicrofacetModel::PDF(float D, float3 H, float3 V)
{
    float NdH = H.z;
    if (NdH <= 0.0f)
        return 0.0f;
    return D * NdH;
}

#endif