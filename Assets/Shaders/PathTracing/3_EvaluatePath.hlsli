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

void EvaluatePathAncillaryVertex(
    inout PathState pathState,
    PathVertexInfo vertex,

    inout float logPDF)
{
    if (vertex.Type == VertexType::eEnvironment)
    {
        float3 L_direct = pathState.Beta * vertex.DirectContribution;
        pathState.Lo += ApplyFireflyThreshold(L_direct, gSettings.FireflyThreshold);

        DBG_OUTPUT3(L_direct, MissContrib);
        DBG_OUTPUT1(1.0f, KilledByMiss);
        return;
    }

    float3 wo = -pathState.Desc.Direction;
    if (vertex.Type == VertexType::eEmission)
    {
        float3 L_sample = pathState.Beta * vertex.DirectContribution;
        pathState.Lo += ApplyFireflyThreshold(L_sample, gSettings.FireflyThreshold);

        DBG_OUTPUT1(1.0f, KilledByEmission);
        return;
    }

    if (vertex.Type != VertexType::eStandard)
        return;

    if (vertex.Lobe == LobeTag::eNEE)
    {
        float3 L_sample = pathState.Beta * vertex.DirectContribution / float(gSettings.DirectNumSamples);
        pathState.Lo += ApplyFireflyThreshold(L_sample, gSettings.FireflyThreshold);

        logPDF += log(vertex.PDF_NEE);
    }
    else
    {
        pathState.Beta *= vertex.IndirectContribution;

        logPDF += log(vertex.PDF_BXDF);
    }

    pathState.IsPrimaryRay = false;
}

void EvaluatePath(PathVertexList vertexList, out float3 Lo, out float pdf)
{
    PathState pathState = CreatePathState(0, 0);

    float logPDF = 0.0f;

    for (uint i = 0; i < vertexList.NumVertices; i++)
    {
        pathState.RaySegmentIdx = i;

        PathVertexInfo vertex = vertexList.Array[i];

        EvaluatePathAncillaryVertex(pathState, vertex, logPDF);
    }

    Lo = pathState.Lo;
    pdf = exp(logPDF);
}

#endif