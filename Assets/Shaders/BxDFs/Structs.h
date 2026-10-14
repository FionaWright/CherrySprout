#ifndef H_BXDF_STRUCTS_H
#define H_BXDF_STRUCTS_H

#include "Utils/HlslGlue.h"

struct BxdfSample
{
    hlsl::float3 Wi;
    bool IsDirac;
    bool IsGlossy; // TODO: Lobe Index?
};

struct BxdfEval
{
    hlsl::float3 Radiance;
    float PDF;
};

#endif