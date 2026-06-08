#ifndef H_MATERIAL_H
#define H_MATERIAL_H

#include "Utils/HlslGlue.h"

struct Material
{
    hlsl::float4 Albedo;

    float EmissiveStrength;
    hlsl::float3 EmissiveColor;

    float TransmissionFactor;
    hlsl::float3 TransmissionColor;

    float Roughness;
    float Metallic;
    float SpecularFactor;
    float AnisoStrength;

    float IOR_N;

    int TexIdxAlbedo;
    int TexIdxNormal;
    int TexIdxRoughness;

    int TexIdxMetallic;
    int TexIdxEmissiveStrength;
    int TexIdxAnisotropy;
    int TexIdxClearcoat;

    int TexIdxClearcoatRoughness;
    int TexIdxClearcoatNormal;
    int TexIdxSheenColor;
    int TexIdxSheenRoughness;

    int TexIdxTransmissionFactor;
    hlsl::float3 p;

#ifdef __cplusplus
    Material()
    {
        Albedo                = { 1, 1, 1, 1 };
        EmissiveColor            = { 1, 1, 1 };
        TransmissionColor        = { 1, 1, 1 };

        Roughness                   = 1.0f;
        Metallic                    = 0.0f;
        SpecularFactor              = 0.0f;
        EmissiveStrength            = 0.0f;
        AnisoStrength               = 0.0f;
        TransmissionFactor          = 0.0f;
        IOR_N                       = 1.5f;

        TexIdxAlbedo                = -1;
        TexIdxNormal                = -1;
        TexIdxRoughness             = -1;
        TexIdxMetallic              = -1;
        TexIdxEmissiveStrength              = -1;
        TexIdxAnisotropy            = -1;
        TexIdxClearcoat             = -1;
        TexIdxClearcoatRoughness    = -1;
        TexIdxClearcoatNormal       = -1;
        TexIdxSheenColor            = -1;
        TexIdxSheenRoughness        = -1;
        TexIdxTransmissionFactor          = -1;
    }
#endif
};

#endif