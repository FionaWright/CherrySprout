#ifndef H_RESTIR_DI_STRUCTS_H
#define H_RESTIR_DI_STRUCTS_H

#include "BxDFs/Structs.h"
#include "Utils/HlslGlue.h"
#include "PathTracing/Structs.h"
#include "PathTracing/GradientDomain/Structs.h"

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
    // Sample:
    LightSample LightSample;

    // Eval:
    LightEval LightEval;
};

struct CandidateGI
{
    // Sample:
    hlsl::float3 SourcePositionBiased; // For Jacobian
    hlsl::float3 DestPosition;
    hlsl::float3 DestNg;

    hlsl::uint IsMiss;
    hlsl::float3 DirectContribution;

    // Eval:
    BxdfEval BxdfEval;
    float Jacobian;
};

// TODO: Need to put vertices in their own buffer to avoid allocating unused vertices
struct CandidatePT
{
    // Sample:
    PathVertexList VertexList;

    // Eval:
    hlsl::float3 Lo;
    float Jacobian;
    float PDF;
};

struct CandidateGPT
{
    // Sample:
    PathVertexList VertexList;
    GradientEstimator Estimator;
    hlsl::uint2 ShiftOffset;
    float OuterJacobian;

    // Eval:
    hlsl::float3 FiniteDifference;
    float PdfRatio;
    float InnerJacobian;
};

typedef Reservoir<CandidateDI> ReservoirDI;
typedef Reservoir<CandidateGI> ReservoirGI;
typedef Reservoir<CandidatePT> ReservoirPT;

typedef Reservoir<CandidateGPT> ReservoirGPT;
typedef Reservoir<CandidateGPT> ReservoirGPT_Pos;
typedef Reservoir<CandidateGPT> ReservoirGPT_Neg;

#endif