#ifndef H_NEE_STRUCTS_H
#define H_NEE_STRUCTS_H

#include "Utils/HlslGlue.h"

struct LightSample
{
    hlsl::float3 Direction;
    float Distance;

    hlsl::float2 UV;
    hlsl::uint Index;
    hlsl::uint SubIndex; // (Emissive primitiveIdx)

    bool IsDelta;
};

struct LightEval
{
    hlsl::float3 Radiance;
    float PDF;
};

#endif