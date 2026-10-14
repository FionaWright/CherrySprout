#ifndef H_RESTIR_DI_STRUCTS_H
#define H_RESTIR_DI_STRUCTS_H

#include "Utils/HlslGlue.h"
#include "PathTracing/Structs.h"

#include "PathTracing/ReSTIR/Reservoir.h"

struct SubReservoir_DI
{
    LightSample LightSample;
    LightEval LightEval;
};

struct SubReservoirInfo_DI
{
    HitInfo HitInfo;

    hlsl::float3 Wo;
    float p;
};

typedef Reservoir<SubReservoir_DI> ReservoirDI;

#endif