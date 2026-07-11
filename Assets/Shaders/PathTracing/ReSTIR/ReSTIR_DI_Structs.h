#ifndef H_RESTIR_DI_STRUCTS_H
#define H_RESTIR_DI_STRUCTS_H

#include "Utils/HlslGlue.h"
#include "PathTracing/Structs.h"

#include "PathTracing/ReSTIR/Reservoir.h"

struct CbvRestirSettings
{
    hlsl::uint ConfidenceCap;
    hlsl::uint NumCandidates;
};

struct LightSampleSelection
{
    hlsl::float3 Direction;
    hlsl::uint LightIndex;

    hlsl::float3 Radiance;
    float PDF;
};

struct LightSampleSelectionInfo
{
    hlsl::float3 Dir_wo;
    hlsl::float3 HitPosOffset;
    HitInfo HitInfo;
};

typedef Reservoir<LightSampleSelection> ReservoirDI;

#endif