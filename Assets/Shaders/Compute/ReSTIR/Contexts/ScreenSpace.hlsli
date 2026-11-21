#ifndef H_RESTIR_SCREEN_SPACE_CONTEXT_H
#define H_RESTIR_SCREEN_SPACE_CONTEXT_H

#include "PathTracing/Structs.h"

#include "PathTracing/Buffers.hlsli"

#include "PathTracing/ReSTIR/ReservoirBuffer.hlsli"
#include "PathTracing/ReSTIR/WRS.hlsli"
#include "Compute/ReSTIR/SeedDecorrelators.h"
#include "Compute/ReSTIR/Utils.hlsli"

template <typename TCandidate>
void GenerateCandidates(RWStructuredBuffer< Reservoir<TCandidate> > buffer, uint2 pixelCoord, uint seedDecorrelator)
{
    uint contextIdx = GetContextBufferIndex(pixelCoord);
    ContextScreenSpace context = gContextsPrimaryRay[contextIdx];
    if (context.IsMiss)
        return;

    RngInfo rngInfo = InitializeRngInfo(pixelCoord, 0, gSettings.FrameIdx, gSettings.Seed);
    DecorrelateRng(rngInfo, seedDecorrelator);
    if (seedDecorrelator == 0)
        SampleRngPrimary(rngInfo);

    Reservoir<TCandidate> reservoir = CreateReservoir<TCandidate>();
    WRS(rngInfo, reservoir, gRestirSettings.GenCandidatesCount, context);

    uint reservoirIdxOut = GetReservoirBufferIndex_InitialCandidatesOutput(pixelCoord);
    buffer[reservoirIdxOut] = reservoir;
}

template <typename TCandidate>
void TemporalReuse(RWStructuredBuffer< Reservoir<TCandidate> > buffer, uint2 pixelCoord, uint seedDecorrelator)
{
    uint contextIdx = GetContextBufferIndex(pixelCoord);
    ContextScreenSpace context = gContextsPrimaryRay[contextIdx];

    uint reservoirIdxIn = GetReservoirBufferIndex_TemporalInput(pixelCoord);
    uint reservoirIdxOut = GetReservoirBufferIndex_TemporalOutput(pixelCoord);

    Reservoir<TCandidate> reservoir = buffer[reservoirIdxIn];

    float2 mv = gGBufferMv[pixelCoord].xy;

    bool prevWasOffscreen = any(mv >= MOTION_VECTOR_OFFSCREEN);
    if (context.IsMiss || prevWasOffscreen)
    {
        buffer[reservoirIdxOut] = reservoir;
        TryPreserveReservoirTemporal(buffer, reservoir, pixelCoord);
        return;
    }

    int2 prevPixelCoord = MotionVectorToPixelCoord(pixelCoord, mv, gSettings.FrameDimensions);

    bool validPrevPixel = all(prevPixelCoord >= 0) && all(prevPixelCoord < int2((float2)gSettings.FrameDimensions));
    if (!validPrevPixel)
    {
        buffer[reservoirIdxOut] = reservoir;
        TryPreserveReservoirTemporal(buffer, reservoir, pixelCoord);
        return;
    }

    uint reservoirIdxPrevIn = GetReservoirBufferIndex_Previous(prevPixelCoord);
    Reservoir<TCandidate> reservoirPrev = buffer[reservoirIdxPrevIn];

    uint contextIdxOther = GetContextBufferIndex_Prev(prevPixelCoord);
    ContextScreenSpace contextOther = gContextsPrimaryRay[contextIdxOther];

    float3 normalMain = context.HitInfo.Ns_ff;
    float3 normalOther = contextOther.HitInfo.Ns_ff;

    float depthMain = LinearizeDepth(context.DepthNonLinear, gSettings.NearPlane, gSettings.FarPlane);
    float depthOther = LinearizeDepth(contextOther.DepthNonLinear, gSettings.NearPlane, gSettings.FarPlane);

    bool isValidPrev = true;
    isValidPrev &= !contextOther.IsMiss;
    isValidPrev &= reservoirPrev.W_Y > 0.0f;
    isValidPrev &= reservoirPrev.PHat > 0.0f;
    isValidPrev &= reservoirPrev.Confidence > 0.0f;
    isValidPrev &= PassNormalThreshold(normalMain, normalOther, gRestirSettings.TemporalNormalDivThreshold);
    isValidPrev &= PassDepthThreshold(depthMain, depthOther, gRestirSettings.TemporalDepthDivThreshold);

    // TODO: When MVs added, lack of ping-pong could cause memory collision issues

    if (isValidPrev)
    {
        RngInfo rngInfo = InitializeRngInfo(pixelCoord, 0, gSettings.FrameIdx, gSettings.Seed);
        DecorrelateRng(rngInfo, seedDecorrelator);

        Shift(reservoirPrev.Y, context);

        ReservoirMerge(rngInfo, gRestirSettings.TemporalConfidenceCap, reservoir, reservoirPrev, context);

        SetUCW(reservoir);
    }

    buffer[reservoirIdxOut] = reservoir;
    TryPreserveReservoirTemporal(buffer, reservoir, pixelCoord);
}

template <typename TCandidate>
void SpatialReuse(RWStructuredBuffer< Reservoir<TCandidate> > buffer, uint2 pixelCoord, uint seedDecorrelator)
{
    uint reservoirIdxIn = GetReservoirBufferIndex_SpatialInput(pixelCoord);
    uint reservoirIdxOut = GetReservoirBufferIndex_SpatialOutput(pixelCoord);
    Reservoir<TCandidate> reservoir = buffer[reservoirIdxIn];

    uint contextIdx = GetContextBufferIndex(pixelCoord);
    ContextScreenSpace context = gContextsPrimaryRay[contextIdx];
    if (context.IsMiss)
    {
        buffer[reservoirIdxOut] = reservoir;
        return;
    }

    float3 normalMain = context.HitInfo.Ns_ff;
    float depthMain = LinearizeDepth(context.DepthNonLinear, gSettings.NearPlane, gSettings.FarPlane);

    RngInfo rngInfo = InitializeRngInfo(pixelCoord, 0, gSettings.FrameIdx, gSettings.Seed);
    DecorrelateRng(rngInfo, seedDecorrelator);

    for (uint i = 0; i < gRestirSettings.SpatialNumSamples; i++)
    {
        int2 coord;
        {
            float u1 = Rand01(rngInfo);
            float u2 = Rand01(rngInfo);
            coord = SampleDisc(u1, u2, pixelCoord, gRestirSettings.SpatialRadius);
            coord.x = max(0, min(coord.x, gSettings.FrameDimensions.x - 1));
            coord.y = max(0, min(coord.y, gSettings.FrameDimensions.y - 1));

            if (all(coord == pixelCoord))
                continue;
        }

        ContextScreenSpace contextOther;
        {
            uint contextIdxOther = GetContextBufferIndex_Prev(coord);
            contextOther = gContextsPrimaryRay[contextIdxOther];
            if (contextOther.IsMiss)
                continue;
        }

        float3 normalOther = contextOther.HitInfo.Ns_ff;
        if (!PassNormalThreshold(normalMain, normalOther, gRestirSettings.SpatialNormalDivThreshold))
            continue;

        float depthOther = LinearizeDepth(contextOther.DepthNonLinear, gSettings.NearPlane, gSettings.FarPlane);
        if (!PassDepthThreshold(depthMain, depthOther, gRestirSettings.SpatialDepthDivThreshold))
            continue;

        uint idx = GetReservoirBufferIndex_SpatialInput(coord);
        Reservoir<TCandidate> reservoirNeighbour = buffer[idx];

        Shift(reservoirNeighbour.Y, context);
        ReservoirMerge(rngInfo, gRestirSettings.SpatialConfidenceCap, reservoir, reservoirNeighbour, context);
    }

    SetUCW(reservoir);

    buffer[reservoirIdxOut] = reservoir;
    TryPreserveReservoirSpatial(buffer, reservoir, pixelCoord);
}

#endif