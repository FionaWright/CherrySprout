#ifndef H_MATERIAL_H
#define H_MATERIAL_H

#include "Utils/HlslGlue.h"

struct Material
{
    hlsl::float4 BaseColor = { 1, 1, 1, 1 };
    hlsl::float3 EmissiveColor = { 1, 1, 1 };

    float Roughness = 1.0f;
    float Metallic = 0.0f;
    float SpecularFactor = 0.0f;
    float EmissiveStrength = 0.0f;
    float AnisoStrength = 0.0f;
    float TransmissionFactor = 0.0f;
    float IOR_N = 1.5f;

    int TexIdxAlbedo = -1;
    int TexIdxNormal = -1;
};

#endif