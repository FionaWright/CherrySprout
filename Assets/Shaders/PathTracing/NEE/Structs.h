#ifndef H_NEE_STRUCTS_H
#define H_NEE_STRUCTS_H

#include "Utils/HlslGlue.h"

struct LightSample
{
    hlsl::uint Index;
    hlsl::float3 p;

    hlsl::float3 BaryOrUV; // Emissive Bary or Env Map UV
    hlsl::uint SubIndex; // Emissive PrimitiveIdx
};

struct LightEval
{
    hlsl::float3 Radiance;
    float PDF;

    hlsl::float3 Direction;
    float Distance;

    bool IsDelta; // TODO: IsDelta might actually belong in LightSample. Guess it could go in either as it's dependent on the light index and nothing else
};

#endif