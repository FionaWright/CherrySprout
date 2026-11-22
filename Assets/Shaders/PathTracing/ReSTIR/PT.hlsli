#ifndef H_RESTIR_PT_H
#define H_RESTIR_PT_H

#include "BxDFs/GetBxDF.hlsli"
#include "PathTracing/ReSTIR/ReservoirBuffer.hlsli"

#if FEAT_ADV_D(RestirPT)

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
        float mis_nee; // Always 1, discard
        EvaluateSubPath(X_i.VertexList, X_i.Lo, X_i.PDF, mis_nee);
		X_i.W = X_i.PDF <= 0.0f ? 0.0f : 1.0f / X_i.PDF;
        return;
    }

    uint M = X_i.VertexList.NumVertices;

    float3 Lo[MAX_RAY_DEPTH];
    float pdf[MAX_RAY_DEPTH];

    float w[MAX_RAY_DEPTH];
    float wSum = 0.0f;

    for (uint i = 0; i < M; i++)
    {
		LobeTag prevLobe = X_i.VertexList.Array[i].Lobe;

        bool isTerminalVertex = i == M - 1;
        if (!isTerminalVertex)
        {
            X_i.VertexList.Array[i].Lobe = LobeTag::eNEE; // Current sub-candidate is subpath with NEE at vertex i
        }

		X_i.VertexList.NumVertices = i + 1;

        float mis_nee;
        EvaluateSubPath(X_i.VertexList, Lo[i], pdf[i], mis_nee);
        //Lo[i] *= mis_nee;

        // TODO: THIS IS REQUIRED FOR NEE, NOT AN OPTIMIZATION
        float mis = 1.0f; // TODO: Generate BxDF candidate for each NEE candidate
        float W_X = pdf[i] <= 0.0f ? 0.0f : 1.0f / pdf[i];
        float pHat = Luminance(Lo[i]);

        w[i] = mis * pHat * W_X;
        wSum += w[i];

		X_i.VertexList.Array[i].Lobe = prevLobe;
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
			bool isTerminalVertex = i == M - 1;
        	if (!isTerminalVertex)
            	X_i.VertexList.Array[i].Lobe = LobeTag::eNEE;
            X_i.VertexList.NumVertices = i + 1;

            X_i.Lo = Lo[i];
			X_i.PDF = pdf[i];

            float pHat = Luminance(Lo[i]);
            X_i.W = wSum / (float(M) * pHat);
            break;
        }
    }
}

template <typename TPrimaryRayInfo>
void Generate(inout RngInfo rngInfo, TPrimaryRayInfo primaryRayInfo, out CandidatePT X_i)
{
    float3 origin = NAN;
    PathState pathState = CreatePathState(origin, primaryRayInfo.CenterRayDirection);
    X_i.VertexList = CreatePathVertexList();

    uint2 pixelCoord = uint2(0,0); // TODO: Needed to combine ReSTIR types, ignore for now

    // Primary Ray
    bool proceed;
    {
		pathState.RaySegmentIdx = 0;
        pathState.SetPathFlag(eIsPrimaryRay, true);
        pathState.SetPathFlag(eIsTerminalRay, 0 == gSettings.MaxRayDepth - 1);

        proceed = ResolvePathAncillaryVertex(pathState, X_i.VertexList, rngInfo, primaryRayInfo.HitInfo, primaryRayInfo.IsMiss, pixelCoord);
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

template <typename TPrimaryRayInfo>
void Shift(inout CandidatePT X_i, TPrimaryRayInfo primaryRayInfo)
{
    float3 origin = NAN;
    int2 pixelCoord = int2(0,0);

    PathState pathState = CreatePathState(origin, primaryRayInfo.CenterRayDirection);
    PathStateShifted pathStateShifted = CreatePathStateShifted();

    pathStateShifted.IsUsingNeeMis = false;
    pathStateShifted.DivideLoByPdf = false;

    // Primary Ray
    bool proceed;
    {
		pathState.RaySegmentIdx = 0;
        pathState.SetPathFlag(eIsPrimaryRay, true);
        pathState.SetPathFlag(eIsTerminalRay, 0 == X_i.VertexList.NumVertices - 1);

        proceed = ResolveShiftedPathAncillaryVertex(pathState, pathStateShifted, X_i.VertexList, primaryRayInfo.IsMiss, primaryRayInfo.HitInfo);
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

    X_i.Lo = pathState.Lo;
    X_i.Jacobian = pathStateShifted.Jacobian;

	// TODO: Does it matter that X_i.W doesn't get updated? Only could be used for reservoir MIS weights
    X_i.PDF = X_i.PDF * pathStateShifted.PdfRatio; // pdfMain * (pdfShifted / pdfMain)
    //X_i.W = X_i.PDF <= 0.0f ? 0.0f : 1.0f / X_i.PDF;
}

template <typename TPrimaryRayInfo>
float Target(CandidatePT X_i, TPrimaryRayInfo primaryRayInfo)
{
    return Luminance(X_i.Lo) * X_i.Jacobian;
}

float PDF(CandidatePT X_i)
{
    return X_i.W <= 0.0f ? 0.0f : 1.0f / X_i.W;
}

template <typename TReservoir>
bool TrySampleReservoirPT(
    uint2 pixelCoord,
    out CandidatePT Y,
    out float3 Lo)
{
    uint reservoirIdx = GetReservoirBufferIndex_Final(pixelCoord);
    TReservoir reservoir = gReservoirBufferPT[reservoirIdx];
    Y = reservoir.Y;

	DBG_OUTPUT3(Palette(reservoirIdx),          RESTIR_ReservoirIdx);
    DBG_OUTPUT1(reservoir.WeightSum,            RESTIR_ReservoirWeightSum);
    DBG_OUTPUT1(reservoir.Confidence / 30,      RESTIR_ReservoirConfidence);
    DBG_OUTPUT1(reservoir.W_Y,                  RESTIR_W_Y);
    DBG_OUTPUT1(Y.Jacobian,                     RESTIR_Jacobian);
    DBG_OUTPUT3(Y.Lo,                           RESTIRPT_Lo);

    if (reservoir.Confidence <= 0.0f || reservoir.W_Y <= 0.0f)
        return false;

    Lo = Y.Lo * reservoir.W_Y * Y.Jacobian;

    // Debug
    {
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
#if FEAT_ADV_D(AreaRestir)
    uint reservoirIdx = GetReservoirBufferIndex_Final(pixelCoord);
    ReservoirPT_Area reservoir = gReservoirBufferPT[reservoirIdx];
    if (reservoir.PreCandidate.IsMiss)
    {
        float mis, _;
        float3 L_miss = Miss(reservoir.PreCandidate.CenterRayDirection, 1.0f, true, false, mis, _);
        return mis * L_miss;
    }

    CandidatePT Y;
    float3 Lo;
    bool found = TrySampleReservoirPT<ReservoirPT_Area>(pixelCoord, Y, Lo);
#else
    uint contextIdx = GetContextBufferIndex(pixelCoord);
    ContextSSPoint context = gContextsPrimaryRay[contextIdx];
    if (context.IsMiss)
    {
        float mis, _;
        float3 L_miss = Miss(context.CenterRayDirection, 1.0f, true, false, mis, _);
        return mis * L_miss;
    }

    CandidatePT Y;
    float3 Lo;
    bool found = TrySampleReservoirPT<ReservoirPT>(pixelCoord, Y, Lo);
#endif

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

template <typename TPrimaryRayInfo> void Generate(inout RngInfo rngInfo, TPrimaryRayInfo primaryRayInfo, out CandidatePT X_i) { X_i = (CandidatePT)0; }
template <typename TPrimaryRayInfo> void Shift(inout CandidatePT X_i, TPrimaryRayInfo primaryRayInfo) {}
template <typename TPrimaryRayInfo> float Target(CandidatePT X_i, TPrimaryRayInfo primaryRayInfo) { return NAN; }
float PDF(CandidatePT X_i) { return NAN; }
bool TrySampleReservoirPT(
    HitInfo hitInfo,
    uint2 pixelCoord,
    float3 wo,
    out CandidatePT Y,
    out float3 Lo) { Y = (CandidatePT)0; Lo = NAN; return NAN; }
float3 SampleRestirPT(uint2 pixelCoord) { return NAN; }

#endif

#endif