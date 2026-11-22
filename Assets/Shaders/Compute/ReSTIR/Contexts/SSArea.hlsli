#ifndef H_RESTIR_SCREEN_SPACE_CONTEXT_H
#define H_RESTIR_SCREEN_SPACE_CONTEXT_H

#include "PathTracing/Structs.h"

#include "PathTracing/Buffers.hlsli"

#include "PathTracing/ReSTIR/ReservoirBuffer.hlsli"
#include "PathTracing/ReSTIR/WRS.hlsli"
#include "Compute/ReSTIR/SeedDecorrelators.h"
#include "Compute/ReSTIR/Utils.hlsli"

#if FEAT_ADV_D(AreaRestir)
#    include "Compute/ReSTIR/PreCandidates/Area.hlsli"
#else
#    include "Compute/ReSTIR/PreCandidates/Point.hlsli"
#endif

template <typename TReservoir>
void GenerateCandidates(RWStructuredBuffer< TReservoir > buffer, uint2 pixelCoord, uint seedDecorrelator)
{
    ContextSSArea context;

    RngInfo rngInfo = InitializeRngInfo(pixelCoord, 0, gSettings.FrameIdx, gSettings.Seed);
    DecorrelateRng(rngInfo, seedDecorrelator);

    TReservoir reservoir = INIT0(TReservoir);
    WRS(rngInfo, reservoir, pixelCoord, gRestirSettings.GenCandidatesCount, context);

    uint reservoirIdxOut = GetReservoirBufferIndex_InitialCandidatesOutput(pixelCoord);
    buffer[reservoirIdxOut] = reservoir;
}

template <typename TReservoir>
void TemporalReuse(RWStructuredBuffer< TReservoir > buffer, uint2 pixelCoord, uint seedDecorrelator)
{
    ContextSSArea context;

    uint reservoirIdxIn = GetReservoirBufferIndex_TemporalInput(pixelCoord);
    uint reservoirIdxOut = GetReservoirBufferIndex_TemporalOutput(pixelCoord);

    TReservoir reservoir = buffer[reservoirIdxIn];

    float2 mv = gGBufferMv[pixelCoord].xy;

    bool prevWasOffscreen = any(mv >= MOTION_VECTOR_OFFSCREEN);
    if (reservoir.PreCandidate.IsMiss || prevWasOffscreen)
    {
        buffer[reservoirIdxOut] = reservoir;
        TryPreserveReservoirTemporal(buffer, reservoir, pixelCoord);
        return;
    }

	RngInfo rngInfo = InitializeRngInfo(pixelCoord, 0, gSettings.FrameIdx, gSettings.Seed);
    DecorrelateRng(rngInfo, seedDecorrelator);

    int2 prevPixelCoord;
    TReservoir reservoirPrev;
	if (!TryFindTemporalReservoir(rngInfo, buffer, context, reservoir.PreCandidate, pixelCoord, mv, prevPixelCoord, reservoirPrev))
	{
		buffer[reservoirIdxOut] = reservoir;
        TryPreserveReservoirTemporal(buffer, reservoir, pixelCoord);
        // TODO: If anything in reservoir changes, might need SetUCW()
        return;
	}

	// Note: Depth/Normal rejections disabled as recommended by paper

    bool isValidPrev = true;
    isValidPrev &= !reservoirPrev.PreCandidate.IsMiss;
    isValidPrev &= reservoirPrev.W_Y > 0.0f;
    isValidPrev &= reservoirPrev.PHat > 0.0f;
    isValidPrev &= reservoirPrev.Confidence > 0.0f;

    // TODO: When MVs added, lack of ping-pong could cause memory collision issues

    if (isValidPrev)
    {
        Shift(reservoirPrev.Y, reservoir.PreCandidate);

        ReservoirMerge(rngInfo, gRestirSettings.TemporalConfidenceCap, reservoir, reservoirPrev, reservoir.PreCandidate);

        SetUCW(reservoir);
    }

    buffer[reservoirIdxOut] = reservoir;
    TryPreserveReservoirTemporal(buffer, reservoir, pixelCoord);
}

template <typename TReservoir>
void SpatialReuse(RWStructuredBuffer< TReservoir > buffer, uint2 pixelCoord, uint seedDecorrelator)
{
	ContextSSArea context;

    uint reservoirIdxIn = GetReservoirBufferIndex_SpatialInput(pixelCoord);
    uint reservoirIdxOut = GetReservoirBufferIndex_SpatialOutput(pixelCoord);
    TReservoir reservoir = buffer[reservoirIdxIn];

    if (reservoir.PreCandidate.IsMiss)
    {
        buffer[reservoirIdxOut] = reservoir;
        return;
    }

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

        uint idx = GetReservoirBufferIndex_SpatialInput(coord);
        TReservoir reservoirNeighbour = buffer[idx];

        if (reservoirNeighbour.W_Y <= 0.0f || reservoirNeighbour.Confidence <= 0.0f)
            continue;

        Shift(reservoirNeighbour.Y, reservoir.PreCandidate);
        ReservoirMerge(rngInfo, gRestirSettings.SpatialConfidenceCap, reservoir, reservoirNeighbour, reservoir.PreCandidate);
    }

    SetUCW(reservoir);

    buffer[reservoirIdxOut] = reservoir;
    TryPreserveReservoirSpatial(buffer, reservoir, pixelCoord);
}

#endif