#ifndef H_RESTIR_DI_STRUCTS_H
#define H_RESTIR_DI_STRUCTS_H

#include "BxDFs/Structs.h"
#include "Utils/HlslGlue.h"
#include "PathTracing/Structs.h"

#include "PathTracing/ReSTIR/Reservoir.h"

// Sub-reservoir must be pixel independent

struct CandidateDI
{
    LightSample LightSample;
    LightEval LightEval;
};

// Note: HitInfo required due to BXDF Eval
// TODO: Is there any way to avoid this cleanly?
// Note: Only a single HitInfo is needed per pixel per frame. Could have ReservoirInfo buffer?
struct ContextDI
{
    HitInfo HitInfo;

    hlsl::float3 Wo;
};

struct SubReservoir_GI
{
    hlsl::float3 Position;
    hlsl::float3 Ns_ff;
    hlsl::float3 Ng_ff;
    hlsl::uint MaterialIndex;

    BxdfEval BxdfEval;
};

struct SubReservoirInfo_GI
{
    HitInfo HitInfo;

    hlsl::float3 Wo;
};

typedef Reservoir<CandidateDI, ContextDI> ReservoirDI;
typedef Reservoir<SubReservoir_GI, SubReservoirInfo_GI> ReservoirGI;

#endif