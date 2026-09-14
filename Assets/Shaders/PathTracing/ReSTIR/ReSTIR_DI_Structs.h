#ifndef H_RESTIR_DI_STRUCTS_H
#define H_RESTIR_DI_STRUCTS_H

#include "Utils/HlslGlue.h"
#include "PathTracing/Structs.h"

#include "PathTracing/ReSTIR/Reservoir.h"

struct LightSampleSelectionInfo
{
    hlsl::float3 Dir_wo;
    hlsl::float3 HitPos;
    hlsl::float3 HitPosOffset;
    HitInfo HitInfo;
};

typedef Reservoir<LightSample> ReservoirDI;

#endif