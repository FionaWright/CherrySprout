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

    float pdfPathLog = 0.0f;
    for (uint i = 0; i < X_i.VertexList.NumVertices; i++)
    {
        if (X_i.VertexList.Array[i].Type == VertexType::eStandard)
            pdfPathLog += log(X_i.VertexList.Array[i].PDF);
    }
    float pdf = exp(pdfPathLog);

    X_i.VertexList = vertexList;
    X_i.Lo = pathState.Lo;
    X_i.Jacobian = 1.0f;
    X_i.PDF = pdf;
}

void ReEvaluate(inout CandidatePT X_i, ContextPrimaryRay context)
{
    float3 origin = NAN;
    int2 pixelCoord = int2(0,0);

    PathState pathState = CreatePathState(origin, -context.Wo);
    PathStateShifted pathStateShifted = CreatePathStateShifted();

    pathState.DivideByPdfEnabled = false;

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

    if (!pathStateShifted.IsSymmetric) // TODO: Successful asymmetric shifted paths can be used
    {
        X_i.Lo = 0.0f;
        return;
    }

    float pdfRatio = pathStateShifted.PdfRatio * pathStateShifted.Jacobian;
    float m = BalanceHeuristicRatio(pdfRatio, 1, 1); // TODO

    X_i.Lo = pathState.Lo * m;
    X_i.Jacobian = pathStateShifted.Jacobian;
    X_i.PDF = pathStateShifted.PdfRatio * X_i.PDF;
}

float Target(CandidatePT X_i, ContextPrimaryRay context)
{
    return Luminance(X_i.Lo) * X_i.Jacobian;
}

float PDF(CandidatePT X_i, ContextPrimaryRay context)
{
    return X_i.PDF;
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

    if (reservoir.Confidence <= 0.0f || reservoir.W_Y <= 0.0f)
        return false;

    Lo = Y.Lo * reservoir.W_Y * Y.Jacobian; // TODO: Applying jacobian twice?

    // Debug
    {
        DBG_OUTPUT3(Palette(reservoirIdx),          RESTIR_ReservoirIdx);
        DBG_OUTPUT1(reservoir.WeightSum,            RESTIR_ReservoirWeightSum);
        DBG_OUTPUT1(reservoir.Confidence / 30,      RESTIR_ReservoirConfidence);
        DBG_OUTPUT1(reservoir.W_Y,                  RESTIR_W_Y);
        DBG_OUTPUT1(Y.Jacobian,                     RESTIR_Jacobian);

        DBG_OUTPUT3(Y.Lo,                                             RESTIRPT_Lo);
        DBG_OUTPUT3(Palette(Y.VertexList.NumVertices),                RESTIRPT_NumVertices);
        DBG_OUTPUT3(Palette((uint)Y.VertexList.Array[0].Type),        RESTIRPT_V1Type);
        DBG_OUTPUT3(Y.VertexList.Array[0].Ng_ff,                      RESTIRPT_V1Ng_ff);
        DBG_OUTPUT3(Palette((uint)Y.VertexList.Array[0].Lobe),        RESTIRPT_V1Lobe);
        DBG_OUTPUT3(Y.VertexList.Array[0].Wo,                         RESTIRPT_V1Wo);
        DBG_OUTPUT3(Y.VertexList.Array[0].Wi,                         RESTIRPT_V1Wi);
        DBG_OUTPUT3(Y.VertexList.Array[0].IndirectContribution,       RESTIRPT_V1Indirect);
        DBG_OUTPUT3(Y.VertexList.Array[0].DirectContribution,         RESTIRPT_V1Direct);
        DBG_OUTPUT1(Y.VertexList.Array[0].ShadowFactor,               RESTIRPT_V1ShadowFactor);
        if (Y.VertexList.NumVertices >= 2)
        {
            DBG_OUTPUT3(Palette((uint)Y.VertexList.Array[1].Type),        RESTIRPT_V2Type);
            DBG_OUTPUT3(Y.VertexList.Array[1].Ng_ff,                      RESTIRPT_V2Ng_ff);
            DBG_OUTPUT3(Palette((uint)Y.VertexList.Array[1].Lobe),        RESTIRPT_V2Lobe);
            DBG_OUTPUT3(Y.VertexList.Array[1].Wo,                         RESTIRPT_V2Wo);
            DBG_OUTPUT3(Y.VertexList.Array[1].Wi,                         RESTIRPT_V2Wi);
            DBG_OUTPUT3(Y.VertexList.Array[1].IndirectContribution,       RESTIRPT_V2Indirect);
            DBG_OUTPUT3(Y.VertexList.Array[1].DirectContribution,         RESTIRPT_V2Direct);
            DBG_OUTPUT1(Y.VertexList.Array[1].ShadowFactor,               RESTIRPT_V2ShadowFactor);
        }

        DBG_ASSERT_GE(reservoir.W_Y, 0.0f,          RESTIR_W_Y_RANGE);
        DBG_ASSERT_GE(reservoir.WeightSum, 0.0f,    RESTIR_WEIGHT_SUM_RANGE);
    }

    return true;
}

float3 SampleRestirPT(uint2 pixelCoord)
{
    uint contextIdx = GetContextBufferIndex(pixelCoord);
    ContextPrimaryRay context = gContextsPrimaryRay[contextIdx];
    if (context.IsMiss)
    {
        return Miss(-context.Wo, 1.0f, true, false);
    }

    CandidatePT Y;
    float3 Lo;
    bool found = TrySampleReservoirPT(context.HitInfo, pixelCoord, context.Wo, Y, Lo);

    DBG_OUTPUT_DUMP_DISABLE();
    {
        DBG_OUTPUT1(0.0f, RESTIR_ReservoirWeightSum);
        DBG_OUTPUT1(0.0f, RESTIR_ReservoirConfidence);
        DBG_OUTPUT1(0.0f, RESTIR_W_Y);
        DBG_OUTPUT1(0.0f, RESTIR_Jacobian);
        DBG_OUTPUT1(0.0f, RESTIRPT_Lo);
        DBG_OUTPUT1(0.0f, RESTIRPT_NumVertices);
        DBG_OUTPUT1(0.0f, RESTIRPT_V1Type);
        DBG_OUTPUT1(0.0f, RESTIRPT_V1Ng_ff);
        DBG_OUTPUT1(0.0f, RESTIRPT_V1Lobe);
        DBG_OUTPUT1(0.0f, RESTIRPT_V1Wo);
        DBG_OUTPUT1(0.0f, RESTIRPT_V1Wi);
        DBG_OUTPUT1(0.0f, RESTIRPT_V1Indirect);
        DBG_OUTPUT1(0.0f, RESTIRPT_V1Direct);
        DBG_OUTPUT1(0.0f, RESTIRPT_V1ShadowFactor);
        DBG_OUTPUT1(0.0f, RESTIRPT_V2Type);
        DBG_OUTPUT1(0.0f, RESTIRPT_V2Ng_ff);
        DBG_OUTPUT1(0.0f, RESTIRPT_V2Lobe);
        DBG_OUTPUT1(0.0f, RESTIRPT_V2Wo);
        DBG_OUTPUT1(0.0f, RESTIRPT_V2Wi);
        DBG_OUTPUT1(0.0f, RESTIRPT_V2Indirect);
        DBG_OUTPUT1(0.0f, RESTIRPT_V2Direct);
        DBG_OUTPUT1(0.0f, RESTIRPT_V2ShadowFactor);
    }
    DBG_OUTPUT_DUMP_ENABLE();

    if (found)
        return Lo;

    return float3(0, 0, 0); // TODO
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
float3 SampleRestirPT(uint2 pixelCoord) { return NAN; }

#endif

#endif