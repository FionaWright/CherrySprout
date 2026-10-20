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
// If I do use ContextBuffer, should be used instead of tracing primary ray
// Maybe PT should have separate TracePrimaryRay pass to build context buffer when using ReSTIR?
struct ContextDI
{
    HitInfo HitInfo;

    hlsl::float3 Wo;
};

struct CandidateGI
{
    hlsl::float3 SourcePositionBiased; // For Jacobian
    hlsl::float3 DestPosition;
    hlsl::float3 Ns_ff;
    hlsl::float3 Ng_ff;

    hlsl::uint IsMiss;
    hlsl::float3 DirectContribution;

    BxdfEval BxdfEval;
};

struct ContextGI
{
    HitInfo HitInfo;

    hlsl::float3 Wo;
};

typedef Reservoir<CandidateDI, ContextDI> ReservoirDI;
typedef Reservoir<CandidateGI, ContextGI> ReservoirGI;

#endif