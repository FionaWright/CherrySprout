#ifndef H_RESTIR_PT_H
#define H_RESTIR_PT_H

#include "BxDFs/GetBxDF.hlsli"
#include "PathTracing/ReSTIR/ReservoirBuffer.hlsli"

#if FEAT_CORE_D(RestirPT)

#include "PathTracing/RandomSample.hlsli"
#include "PathTracing/NEE/SampleNEE.hlsli"
#include "PathTracing/VisibilityRays.hlsli"
#include "PathTracing/HitInfo/GetHitInfo.hlsli"
#include "PathTracing/4_Miss.hlsli"
#include "PathTracing/Shift/ShiftMapping/Components/Reconnect.hlsli"
#include "PathTracing/Shift/ShiftMapping/Components/ReconnectEnv.hlsli"
#include "PathTracing/Shift/TraceShiftedPath.hlsli"
#include "PathTracing/3_TracePath.hlsli"

#include "PathTracing/ReSTIR/Structs.h"
#include "PathTracing/ReSTIR/WRS.hlsli"
#include "Utils/Debug/Palette.h"
#include "PathTracing/Debug/Assert.hlsli"

void Generate(inout RngInfo rngInfo, ContextPrimaryRay context, out CandidatePT X_i)
{
    float3 origin = NAN;
    PathState pathState = CreatePathState(origin, -context.Wo);
    PathVertexList vertexList = CreatePathVertexList();

    pathState.DivideByPdfEnabled = false;

    uint2 pixelCoord = uint2(0,0); // TODO: Needed to combine ReSTIR types, ignore for now

    // Primary Ray
    bool proceed;
    {
        proceed = ResolvePathAncillaryVertex(pathState, vertexList, rngInfo, context.HitInfo, context.IsMiss, pixelCoord);
    }

    if (proceed)
    {
        for (uint i = 1; i < gSettings.MaxRayDepth; i++)
        {
            pathState.RaySegmentIdx = i;

            if (!TracePathAncillary(pathState, vertexList, rngInfo, pixelCoord))
                break;
        }
    }

    X_i.VertexList = vertexList;
    X_i.Lo = pathState.Lo;
    X_i.Jacobian = 1.0f;
}

void ReEvaluate(inout CandidatePT X_i, ContextPrimaryRay context)
{
    float3 origin = NAN;
    int2 pixelCoord = int2(0,0);

    PathState pathState = CreatePathState(origin, -context.Wo);
    PathStateShifted pathStateShifted = CreatePathStateShifted();

    // Primary Ray
    bool proceed;
    {
        proceed = ResolveShiftedPathAncillaryVertex(pathState, pathStateShifted, X_i.VertexList, context.IsMiss, context.HitInfo);
    }

    if (proceed)
    {
        DBG_ASSERT_LE(X_i.VertexList.NumVertices, gSettings.MaxRayDepth, MAX_RAY_DEPTH_SHIFT_ID);

        for (uint i = 1; i < X_i.VertexList.NumVertices; i++)
        {
            pathState.RaySegmentIdx = i;

            if (!TraceShiftedPathAncillary(pathState, pathStateShifted, X_i.VertexList, pixelCoord))
                break;
        }
    }

    if (!pathStateShifted.IsSymmetric) // TODO
    {
        X_i.Lo = 0.0f;
        X_i.Jacobian = 1.0f;
        return;
    }

    X_i.Lo = pathState.Lo;
    X_i.Jacobian = pathStateShifted.Jacobian;
}

float Target(CandidatePT X_i, ContextPrimaryRay context)
{
    return Luminance(X_i.Lo) * X_i.Jacobian;
}

float PDF(CandidatePT X_i, ContextPrimaryRay context)
{
    // TODO: Not happy about this at all
    // 1. Ignores primary RNG and NEE RNG
    // 2. Prone to FP-Precision issues

	//return 1.0f;

    float pdfPath = 1.0f;
    for (uint i = 0; i < X_i.VertexList.NumVertices; i++)
    {
        pdfPath *= X_i.VertexList.Array[i].PDF;
    }
    return pdfPath;
}

bool TrySampleReservoirPT(
    HitInfo hitInfo,
    uint2 pixelCoord,
    float3 wo,
    out CandidatePT Y,
    out float3 Lo)
{
    uint reservoirIdx = GetReservoirBufferIndex_Final(pixelCoord);
    ReservoirPT reservoir = gReservoirBufferPT[reservoirIdx];
    Y = reservoir.Y;

    // Debug
    {
        DBG_OUTPUT3(Palette(reservoirIdx),          RESTIR_ReservoirIdx);
        DBG_OUTPUT1(reservoir.WeightSum,            RESTIR_ReservoirWeightSum);
        DBG_OUTPUT1(reservoir.Confidence / 30,      RESTIR_ReservoirConfidence);
        DBG_OUTPUT1(reservoir.W_Y,                  RESTIR_W_Y);

        DBG_OUTPUT1(Y.Jacobian,                     RESTIRGI_Jacobian); // TODO

        DBG_ASSERT_GE(reservoir.W_Y, 0.0f,          RESTIR_W_Y_RANGE);
        DBG_ASSERT_GE(reservoir.WeightSum, 0.0f,    RESTIR_WEIGHT_SUM_RANGE);
    }

    if (reservoir.Confidence <= 0.0f || reservoir.W_Y <= 0.0f)
        return false;

    Lo = Y.Lo * Y.Jacobian;

    return true;
}

#else

void Generate(inout RngInfo rngInfo, ContextPrimaryRay context, out CandidatePT X_i) { X_i = (CandidatePT)0; }
void ReEvaluate(inout CandidatePT X_i, ContextPrimaryRay context) {}
float Target(CandidatePT X_i, ContextPrimaryRay context) { return NAN; }
float PDF(CandidatePT X_i, ContextPrimaryRay context) { return NAN; }
bool TrySampleReservoirPT(
    HitInfo hitInfo,
    uint2 pixelCoord,
    float3 wo,
    out CandidatePT Y,
    out float3 Lo) { Y = (CandidatePT)0; Lo = NAN; return NAN; }

#endif

#endif