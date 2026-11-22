#ifndef H_RESTIR_DI_STRUCTS_H
#define H_RESTIR_DI_STRUCTS_H

#include "BxDFs/Structs.h"
#include "Utils/HlslGlue.h"
#include "PathTracing/Structs.h"
#include "PathTracing/GradientDomain/Structs.h"

#include "PathTracing/ReSTIR/Reservoir.h"

struct ContextSSPoint
{
    HitInfo HitInfo; // TODO: Very heavy, use material index?
    hlsl::float3 CenterRayDirection;
    hlsl::uint IsMiss;
    float DepthNonLinear;
};

struct ContextSSArea
{
    // None
    hlsl::uint Dummy;
};

struct PreCandidatePoint
{
    // None
    // Implicit pixel position (0.5,0.5)
    hlsl::uint Dummy;

    // TODO: Make specialized reservoirs that either have or don't have pre candidates
};

struct PreCandidateArea
{
    // Sample:
    RngValuesPrimary RngPrimary;

    // Eval:
    HitInfo HitInfo; // TODO: Very heavy, use material index?
    hlsl::float3 CenterRayDirection;
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
    float W;
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

typedef Reservoir<CandidateDI, PreCandidatePoint> ReservoirDI;
typedef Reservoir<CandidateGI, PreCandidatePoint> ReservoirGI;
typedef Reservoir<CandidatePT, PreCandidatePoint> ReservoirPT;
typedef Reservoir<CandidateGPT, PreCandidatePoint> ReservoirGPT;
typedef Reservoir<CandidateGPT, PreCandidatePoint> ReservoirGPT_Pos; // TODO
typedef Reservoir<CandidateGPT, PreCandidatePoint> ReservoirGPT_Neg;

typedef Reservoir<CandidateDI, PreCandidateArea> ReservoirDI_Area;
typedef Reservoir<CandidateGI, PreCandidateArea> ReservoirGI_Area;
typedef Reservoir<CandidatePT, PreCandidateArea> ReservoirPT_Area;
typedef Reservoir<CandidateGPT, PreCandidateArea> ReservoirGPT_Area;
typedef Reservoir<CandidateGPT, PreCandidateArea> ReservoirGPT_Pos_Area;
typedef Reservoir<CandidateGPT, PreCandidateArea> ReservoirGPT_Neg_Area;

#endif