#ifndef H_RESTIR_PT_H
#define H_RESTIR_PT_H

#include "BxDFs/GetBxDF.hlsli"
#include "PathTracing/ReSTIR/ReservoirBuffer.hlsli"

#if FEAT_CORE_D(RestirPT)

#include "PathTracing/RandomSample.hlsli"
#include "PathTracing/HitInfo/GetHitInfo.hlsli"
#include "PathTracing/4_Miss.hlsli"
#include "PathTracing/Shift/ShiftMapping/Components/Reconnect.hlsli"
#include "PathTracing/Shift/ShiftMapping/Components/ReconnectEnv.hlsli"
#include "PathTracing/Shift/TraceShiftedPath.hlsli"
#include "PathTracing/3_TracePath.hlsli"
#include "PathTracing/Shift/EvaluateSubPath.hlsli"

#include "PathTracing/ReSTIR/Structs.h"
#include "PathTracing/ReSTIR/WRS.hlsli"
#include "Utils/Debug/Palette.h"
#include "PathTracing/Debug/Assert.hlsli"

void selectSubPathRIS(inout RngInfo rngInfo, inout CandidatePT X_i)
{
    if (!FEAT_CORE(NEE))
    {
        float finalMIS; // Always 1, discard
        EvaluateSubPath(X_i.VertexList, X_i.Lo, X_i.PDF, finalMIS);
        return;
    }

    uint M = X_i.VertexList.NumVertices;

    float3 Lo[MAX_RAY_DEPTH];
    float pdf[MAX_RAY_DEPTH];

    float w[MAX_RAY_DEPTH];
    float wSum = 0.0f;

    for (uint i = 0; i < M; i++)
    {
        bool terminalVertex = i == M - 1;
        if (!terminalVertex)
        {
            X_i.VertexList.Array[i].Lobe = LobeTag::eNEE;
            X_i.VertexList.NumVertices = i + 1;
        }

        float finalMIS;
        EvaluateSubPath(X_i.VertexList, Lo[i], pdf[i], finalMIS);
        //Lo[i] *= finalMIS;

        float mis = 1.0f; // TODO: Generate BxDF candidate for each NEE candidate
        float W_X = pdf[i] <= 0.0f ? 0.0f : 1.0f / pdf[i];
        float pHat = Luminance(Lo[i]);

        w[i] = mis * pHat * W_X;
        wSum += w[i];
    }

    if (wSum <= 0.0f)
    {
        X_i.Lo = 0.0f;
        return;
    }

    float r = Rand01(rngInfo);
    for (uint i = 0; i < M; i++)
    {
        r -= w[i] / wSum;
        if (r <= 0.0f)
        {
            X_i.VertexList.Array[i].Lobe = LobeTag::eNEE;
            X_i.VertexList.NumVertices = i + 1;

            X_i.Lo = Lo[i];

            float pHat = Luminance(Lo[i]);
            float W_Y = wSum / (float(M) * pHat);

            X_i.PDF = 1.0f / W_Y; // TODO
        }
    }
}

void Generate(inout RngInfo rngInfo, ContextPrimaryRay context, out CandidatePT X_i)
{
    float3 origin = NAN;
    PathState pathState = CreatePathState(origin, -context.Wo);
    X_i.VertexList = CreatePathVertexList();

    uint2 pixelCoord = uint2(0,0); // TODO: Needed to combine ReSTIR types, ignore for now

    // Primary Ray
    bool proceed;
    {
        proceed = ResolvePathAncillaryVertex(pathState, X_i.VertexList, rngInfo, context.HitInfo, context.IsMiss, pixelCoord);
    }

    if (proceed)
    {
        for (uint i = 1; i < gSettings.MaxRayDepth; i++)
        {
            pathState.RaySegmentIdx = i;
            pathState.SetPathFlag(eIsPrimaryRay, i == 0);
            pathState.SetPathFlag(eIsTerminalRay, i == gSettings.MaxRayDepth - 1);

            if (!TracePathAncillary(pathState, X_i.VertexList, rngInfo, pixelCoord))
                break;
        }
    }

    X_i.Jacobian = 1.0f;

    // TODO: Can all evaluations be done in one pass?

    selectSubPathRIS(rngInfo, X_i);
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
            pathState.SetPathFlag(eIsPrimaryRay, i == 0);
            pathState.SetPathFlag(eIsTerminalRay, i == X_i.VertexList.NumVertices - 1);

            if (!TraceShiftedPathAncillary(pathState, pathStateShifted, X_i.VertexList, pixelCoord))
                break;
        }
    }

    if (!pathStateShifted.IsSymmetric) // TODO: Successful asymmetric shifted paths can be used?
    {
        X_i.Lo = 0.0f;
        return;
    }

    //float mis = BalanceHeuristicRatio(pathStateShifted.PdfRatio, 1, 1);
    //pathState.Lo *= mis;

    X_i.Lo = pathState.Lo;
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

    Lo = Y.Lo * reservoir.W_Y;

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
        float mis;
        float3 L_miss = Miss(-context.Wo, 1.0f, true, false, mis);
        return mis * L_miss;
    }

    CandidatePT Y;
    float3 Lo;
    bool found = TrySampleReservoirPT(context.HitInfo, pixelCoord, context.Wo, Y, Lo);

    DBG_OUTPUT_DUMP_DISABLE();
    {
        DBG_OUTPUT1(0.0f, KilledByBetaLoss);
        DBG_OUTPUT1(0.0f, KilledByRR);
        DBG_OUTPUT1(0.0f, KilledByMiss);
        DBG_OUTPUT1(0.0f, KilledByEmission);
        DBG_OUTPUT1(0.0f, RESTIR_ReservoirIdx);
        DBG_OUTPUT1(0.0f, RESTIR_ReservoirWeightSum);
        DBG_OUTPUT1(0.0f, RESTIR_ReservoirConfidence);
        DBG_OUTPUT1(0.0f, RESTIR_NdL);
        DBG_OUTPUT1(0.0f, RESTIR_W_Y);
        DBG_OUTPUT1(0.0f, RESTIR_Jacobian);
        DBG_OUTPUT1(0.0f, RESTIRGI_Dir);
        DBG_OUTPUT1(0.0f, RESTIRGI_DirectContrib);
        DBG_OUTPUT1(0.0f, RESTIRGI_BxdfRadiance);
        DBG_OUTPUT1(0.0f, RESTIRGI_BxdfPDF);
        DBG_OUTPUT1(0.0f, RESTIRGI_GiLo);

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