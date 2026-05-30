#ifndef H_MATERIAL_H
#define H_MATERIAL_H

#include "Utils/HlslGlue.h"

struct Material
{
    hlsl::float4 BaseColor;
    hlsl::float4 EmissiveColor;

    float Roughness;
    float Metallic;
    float TransmissionFactor;
    float IOR_N;

    hlsl::uint TexIdxAlbedo;
    hlsl::uint TexIdxNormal;
};

#endif