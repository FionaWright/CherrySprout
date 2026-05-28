#ifndef H_MATERIAL_H
#define H_MATERIAL_H

#include "Utils/HlslGlue.h"

struct Material
{
    hlsl::float4 BaseColor;
    hlsl::float4 EmissiveColor;

    hlsl::float Roughness;
    hlsl::float Metallic;
    hlsl::float TransmissionFactor;
    hlsl::float IOR_N;

    hlsl::uint TexIdxAlbedo;
    hlsl::uint TexIdxNormal;
};

#endif