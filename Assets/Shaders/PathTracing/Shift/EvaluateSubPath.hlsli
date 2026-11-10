#ifndef H_EVALUATE_PATH_H
#define H_EVALUATE_PATH_H

#include "PathTracing/Structs.h"
#include "PathTracing/Utils.hlsli"
#include "PathTracing/4_Hit.hlsli"
#include "PathTracing/4_Miss.hlsli"
#include "PathTracing/4_ComputeRayHit.hlsli"
#include "PathTracing/Debug/PathDumper.hlsli"
#include "PathTracing/Debug/Globals.hlsli"
#include "PathTracing/ReSTIR/GI.hlsli"
#include "PathTracing/Shift/Utils.hlsli"

#include "PathTracing/RandomSample.hlsli"
#include "Utils/HlslUtils.hlsli"

#if PATH_VERTICES_REQUIRED_D

// finalMIS set to final vertex MIS between BXDF/NEE sampling
bool EvaluateSubPathAncillaryVertex(
    inout PathState pathState,
    PathVertexInfo vertex,

    inout float logPdf,
    out float finalMIS)
{
    finalMIS = 1.0f;

    if (vertex.Type == VertexType::eEnvironment)
    {
        float3 L_direct = pathState.Beta * vertex.DirectContribution;
        pathState.Lo += ApplyFireflyThreshold(L_direct, gSettings.FireflyThreshold);

        finalMIS = vertex.MIS;

        DBG_OUTPUT3(L_direct, MissContrib);
        DBG_OUTPUT1(1.0f, KilledByMiss);
        return false;
    }

    float3 wo = -pathState.Desc.Direction;
    if (vertex.Type == VertexType::eEmission)
    {
        float3 L_sample = pathState.Beta * vertex.DirectContribution;
        pathState.Lo += ApplyFireflyThreshold(L_sample, gSettings.FireflyThreshold);

        finalMIS = vertex.MIS;

        DBG_OUTPUT1(1.0f, KilledByEmission);
        return false;
    }

    if (vertex.Type != VertexType::eStandard)
        return false;

    if (vertex.Lobe == LobeTag::eNEE)
    {
        float3 L_sample = pathState.Beta * vertex.DirectContribution / float(gSettings.DirectNumSamples);
        pathState.Lo += ApplyFireflyThreshold(L_sample, gSettings.FireflyThreshold);

        finalMIS = vertex.MIS;

        logPdf += log(vertex.PDF_NEE);
        return false;
    }

    if (vertex.PDF_BXDF <= 0.0f)
        return false;

    {
        pathState.Beta *= vertex.IndirectContribution;

        logPdf += log(vertex.PDF_BXDF);
        return true;
    }
}

void EvaluateSubPath(PathVertexList vertexList, out float3 Lo, out float pdf, out float finalMIS)
{
    finalMIS = 1.0f;

    PathState pathState = CreatePathState(0, 0);

    float logPDF = 0.0f;

    for (uint i = 0; i < vertexList.NumVertices; i++)
    {
        pathState.RaySegmentIdx = i;

        PathVertexInfo vertex = vertexList.Array[i];

        if (!EvaluateSubPathAncillaryVertex(pathState, vertex, logPDF, finalMIS))
            break;
    }

    Lo = pathState.Lo;
    pdf = exp(logPDF);
}

#else

bool EvaluateSubPathAncillaryVertex(
    inout PathState pathState,
    PathVertexInfo vertex,
    inout float logPDF,
    out float finalMIS) { logPDF = NAN; finalMIS = NAN; return false; }

void EvaluateSubPath(PathVertexList vertexList, out float3 Lo, out float pdf, out float finalMIS) { Lo = NAN; pdf = NAN; finalMIS = NAN; }

#endif

#endif