#ifndef H_RESTIR_DI_STRUCTS_H
#define H_RESTIR_DI_STRUCTS_H

#include "BxDFs/Structs.h"
#include "Utils/HlslGlue.h"
#include "PathTracing/Structs.h"
#include "PathTracing/GradientDomain/Structs.h"

#include "PathTracing/ReSTIR/Reservoir.h"

struct ContextScreenSpace
{
    HitInfo HitInfo; // TODO: Very heavy, use material index?

    hlsl::float3 CenterRayDirection;
    hlsl::uint IsMiss;
    float DepthNonLinear;
};

struct PixelInfoPoint
{
    // None
};

struct PixelInfoArea
{
    RngValuesPrimary RngPrimary;
    hlsl::float3 RayDirection;
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

#define ReservoirX Reservoir<TCandidate, PixelInfoPoint>
#define ReservoirX_Area Reservoir<TCandidate, PixelInfoArea>

typedef Reservoir<CandidateDI, PixelInfoPoint> ReservoirDI;
typedef Reservoir<CandidateGI, PixelInfoPoint> ReservoirGI;
typedef Reservoir<CandidatePT, PixelInfoPoint> ReservoirPT;
typedef Reservoir<CandidateGPT, PixelInfoPoint> ReservoirGPT;
typedef Reservoir<CandidateGPT, PixelInfoPoint> ReservoirGPT_Pos; // TODO
typedef Reservoir<CandidateGPT, PixelInfoPoint> ReservoirGPT_Neg;

typedef Reservoir<CandidateDI, PixelInfoArea> ReservoirDI_Area;
typedef Reservoir<CandidateGI, PixelInfoArea> ReservoirGI_Area;
typedef Reservoir<CandidatePT, PixelInfoArea> ReservoirPT_Area;
typedef Reservoir<CandidateGPT, PixelInfoArea> ReservoirGPT_Area;
typedef Reservoir<CandidateGPT, PixelInfoArea> ReservoirGPT_Pos_Area;
typedef Reservoir<CandidateGPT, PixelInfoArea> ReservoirGPT_Neg_Area;

#endif