#ifndef H_MATERIAL_H
#define H_MATERIAL_H

#include "Utils/HlslGlue.h"

struct Material
{
    hlsl::float4 BaseColor;
    hlsl::float3 EmissiveColor;

    float Roughness;
    float Metallic;
    float SpecularFactor;
    float EmissiveStrength;
    float AnisoStrength;
    float TransmissionFactor;
    float IOR_N;

    int TexIdxAlbedo;
    int TexIdxNormal;

#ifdef __cplusplus
    Material()
    {
        BaseColor = { 1, 1, 1, 1 };
        EmissiveColor = { 1, 1, 1 };

        Roughness = 1.0f;
        Metallic = 0.0f;
        SpecularFactor = 0.0f;
        EmissiveStrength = 0.0f;
        AnisoStrength = 0.0f;
        TransmissionFactor = 0.0f;
        IOR_N = 1.5f;

        TexIdxAlbedo = -1;
        TexIdxNormal = -1;
    }
#endif
};

#endif