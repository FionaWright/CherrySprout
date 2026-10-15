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

// Note: HitInfo required due to BXDF Eval
// TODO: Is there any way to avoid this cleanly?
// Note: Only a single HitInfo is needed per pixel per frame. Could have ReservoirInfo buffer?
struct SubReservoirInfo_DI
{
    HitInfo HitInfo;

    hlsl::float3 Wo;
};

typedef Reservoir<SubReservoir_DI, SubReservoirInfo_DI> ReservoirDI;

#endif