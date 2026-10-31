#ifndef H_RESTIR_DI_STRUCTS_H
#define H_RESTIR_DI_STRUCTS_H

#include "BxDFs/Structs.h"
#include "Utils/HlslGlue.h"
#include "PathTracing/Structs.h"

#include "PathTracing/ReSTIR/Reservoir.h"

struct ContextPrimaryRay
{
    HitInfo HitInfo; // TODO: Very heavy, use material index?

    hlsl::float3 Wo;
    hlsl::uint IsMiss;
    float DepthNonLinear;
};

struct CandidateDI
{
    LightSample LightSample;
    LightEval LightEval;
};

struct CandidateGI
{
    hlsl::float3 SourcePositionBiased; // For Jacobian
    hlsl::float3 DestPosition;
    hlsl::float3 DestNg;

    hlsl::uint IsMiss;
    hlsl::float3 DirectContribution;

    // TODO: Differentiate between sample and evaluate candidate members?
    BxdfEval BxdfEval;
    float Jacobian;
};

// TODO: Storing all RNG values rather than initial rng state may not be worth it
struct CandidatePT
{
    PathVertexList VertexList;

    hlsl::float3 Lo;
    float Jacobian;
};

typedef Reservoir<CandidateDI> ReservoirDI;
typedef Reservoir<CandidateGI> ReservoirGI;
typedef Reservoir<CandidatePT> ReservoirPT;

#endif