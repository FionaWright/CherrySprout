#ifndef H_GGX_SMITH_ISO_H
#define H_GGX_SMITH_ISO_H

#define MICROFACET_MODEL_CHOSEN

// The Big One, old:
// https://inria.hal.science/hal-00996995v2

// Simple Modern:
// https://jcgt.org/published/0007/04/01/paper.pdf

// Simple Smith Iso:
// https://auzaiffe.wordpress.com/2024/04/15/vndf-importance-sampling-an-isotropic-distribution/

// Also smith, recent:
// https://arxiv.org/pdf/2306.05044

// Bounded, more advanced?
// https://dl.acm.org/doi/abs/10.1145/3651291

struct MicrofacetModel
{
#include "MicrofacetModels/IMicrofacetModel.hlsli"

    float m_alpha;
    float3 m_V;

    float3 m_H;
};

void MicrofacetModel::Init(float roughness, float3 V)
{
    m_alpha = RoughnessToAlpha(roughness);
    m_V = V;
}

uint GetRngCount() { return 2; }

void MicrofacetModel::InitAniso(HitInfo _) { }

float MicrofacetModel::RoughnessToAlpha(float roughness)
{
    return roughness * roughness;
}

float3 MicrofacetModel::Sample(float3 rng)
{
    float3 Vh = normalize(float3(m_alpha * m_V.xy, m_V.z));

    float l = LengthSquared(Vh.xy);
    float3 T1 = l <= 0.0f ? float3(1,0,0) : float3(-Vh.y, Vh.x, 0) * rsqrt(max(0.0f, l));
    float3 T2 = cross(Vh, T1);

    float r = sqrt(rng.x);
    float phi = 2 * PI * rng.y;
    float t1 = r * cos(phi);
    float t2 = r * sin(phi);
    float s = 0.5f * (1 + Vh.z);

    t2 = (1 - s) * sqrt(max(0.0f, 1.0f - t1*t1)) + s * t2;

    float3 Nh = t1*T1 + t2*T2 + sqrt(max(0.0f, 1.0f - t1*t1 - t2*t2)) * Vh;
    return NormalizeSafe(float3(m_alpha * Nh.xy, max(0.0f, Nh.z)), float3(0,0,1));
}

float MicrofacetModel::D(float3 H)
{
    float a2 = m_alpha * m_alpha;
    float NdH = H.z;

    float denominator = (NdH * NdH * (a2 - 1.0f) + 1.0f);
    return a2 / max(1e-6f, PI * denominator * denominator);
}

float MicrofacetModel::G1(float NdW)
{
    float a2 = m_alpha * m_alpha;

    float denom = NdW + sqrt(max(0.0f, a2 + (1-a2) * NdW * NdW));
    return saturate(2 * NdW / max(1e-6f, denom));
}

float MicrofacetModel::G2(float NdL, float NdV)
{
    return G1(NdL) * G1(NdV);
}

float MicrofacetModel::PDF(float D, float3 H, float3 V)
{
    float NdV = V.z;
    float NdH = H.z;
    float VdH = dot(V, H);

    if (NdH <= 0.0f)
        return 0.0f;

    float Dv = D * G1(NdV) * max(0.0f, VdH) / max(1e-6f, NdV);
    return Dv;
}

#endif