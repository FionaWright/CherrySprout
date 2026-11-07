#ifndef H_SHADOW_MAP_PUNC_INFO_H
#define H_SHADOW_MAP_PUNC_INFO_H

#include "Utils/HlslGlue.h"

// Type learned from corresponding punctual light
struct ShadowMapPuncInfo
{
    hlsl::float4x4 ShadowMatrix;

    hlsl::uint ShadowMapIndex;
    hlsl::float2 TexelSize;
    float FarPlane;
};

#endif