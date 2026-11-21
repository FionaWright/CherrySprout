#ifndef H_SHIFT_STRUCTS_H
#define H_SHIFT_STRUCTS_H

#include "BxDFs/Structs.h"
#include "Utils/HlslGlue.h"

#include "PathTracing/Structs.h"
#include "PathTracing/NEE/Structs.h"

enum class VertexType : hlsl::uint
{
    eUninitialized,
    eStandard,
    eEnvironment,
    eEmission,
};

struct PathVertexInfo
{
    hlsl::float3 Position;
    VertexType Type;

    hlsl::float3 Ng_ff;
    LobeTag Lobe;

    hlsl::float3 Wo;
    float Eta;

    hlsl::float3 Wi;
    float Roughness;

    hlsl::float3 IndirectContribution;
    hlsl::uint InitialRngAncillaryState;

    hlsl::float3 DirectContribution;
    float ShadowFactor;

    LightSample LightSample; // TODO: Multiple NEE samples

    float PDF_BXDF;
    float PDF_NEE;
    float PDF_BXDF_at_NEE; // BxDF eval in light direction
    float MIS_NEE;

    hlsl::float3 InitialBeta;
    float p2;
};

struct PathVertexList
{
    PathVertexInfo Array[MAX_RAY_DEPTH_SHIFT];
    hlsl::uint NumVertices;
};

inline PathVertexList CreatePathVertexList()
{
    PathVertexList vertexList; // Note: Purposefully not initialized
    vertexList.NumVertices = 0;
    return vertexList;
}

enum class ReconnectionState : hlsl::uint
{
    eUnconnected,
    eSemiConnected,  // Connected but has different wo so needs BxDF evals
    eConnected,      // Can reuse Hit contribution
};

struct PathStateShifted
{
    float Jacobian;
    float PathPdfMain;
    float PathPdfShifted;
    float PdfRatio;

    hlsl::uint IsSymmetric;
    hlsl::float3 SubpathGradients;

    hlsl::uint IsUsingNeeMis;
    hlsl::uint DivideLoByPdf;
    ReconnectionState ReconnectionState;
    float p;
};

inline PathStateShifted CreatePathStateShifted()
{
    PathStateShifted pathStateShifted = (PathStateShifted)0;

    pathStateShifted.Jacobian = 1.0f;
    pathStateShifted.PathPdfMain = 1.0f;
    pathStateShifted.PathPdfShifted = 1.0f;
    pathStateShifted.PdfRatio = 1.0f;
    pathStateShifted.IsSymmetric = true;
    pathStateShifted.IsUsingNeeMis = true;
    pathStateShifted.DivideLoByPdf = true;
    pathStateShifted.SubpathGradients = hlsl::float3(0,0,0);
    pathStateShifted.ReconnectionState = ReconnectionState::eUnconnected;

    return pathStateShifted;
}

#endif