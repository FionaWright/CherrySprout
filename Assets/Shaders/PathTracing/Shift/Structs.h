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
    float MIS;
    float p;
};

struct PathVertexList
{
    PathVertexInfo Array[MAX_RAY_DEPTH_SHIFT];
    hlsl::uint NumVertices;
};

inline PathVertexList CreatePathVertexList()
{
    PathVertexList vertexList;
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
    float PdfRatio;
    bool IsSymmetric;
    ReconnectionState ReconnectionState;
};

inline PathStateShifted CreatePathStateShifted()
{
    PathStateShifted pathStateShifted = (PathStateShifted)0;

    pathStateShifted.Jacobian = 1.0f;
    pathStateShifted.PdfRatio = 1.0f;
    pathStateShifted.IsSymmetric = true;
    pathStateShifted.ReconnectionState = ReconnectionState::eUnconnected;

    return pathStateShifted;
}

#endif