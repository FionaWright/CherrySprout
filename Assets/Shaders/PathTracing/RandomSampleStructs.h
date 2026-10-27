#ifndef H_RANDOM_SAMPLE_STRUCTS_H
#define H_RANDOM_SAMPLE_STRUCTS_H

#include "Utils/HlslGlue.h"
#include "Utils/Constants.h"

struct RngValuesPrimary
{
    hlsl::float2 Jitter;
    hlsl::float2 DoF;
};

struct RngValuesAncillary
{
    // === Batch 1 ===
    float AlphaTest;

    struct RngValuesNEE
    {
        float LSD;
        hlsl::float2 Light;
    } NEE[MAX_DIRECT_SAMPLES];

    struct RngValuesBxDF
    {
        hlsl::float3 Microfacet;
        hlsl::float2 LobeSelect;
    } BxDF;
    // ===============

    // === Batch 2 ===
    float RussianRoulette;
    // ===============

    // Note: ReSTIR generates other RNG values that aren't included here
};

struct RngValues
{
    RngValuesPrimary Primary;
    RngValuesAncillary Ancillaries[MAX_RAY_DEPTH];
};

#endif