#ifndef H_RESTIR_SCREEN_SPACE_CONTEXT_H
#define H_RESTIR_SCREEN_SPACE_CONTEXT_H

#include "PathTracing/Structs.h"

#include "PathTracing/Buffers.hlsli"
#include "PathTracing/3_GetPrimaryRay.hlsli"
#include "PathTracing/HitInfo/ReconstructPrimaryRay.hlsli"

#include "PathTracing/ReSTIR/ReservoirBuffer.hlsli"
#include "PathTracing/ReSTIR/WRS.hlsli"
#include "Compute/ReSTIR/SeedDecorrelators.h"
#include "Compute/ReSTIR/Utils.hlsli"

#if FEAT_ADV_D(AreaRestir)
#    include "Compute/ReSTIR/PreCandidates/Area.hlsli"
#else
#    include "Compute/ReSTIR/PreCandidates/Point.hlsli"
#endif

void InitContext(uint2 pixelCoord, out ContextSSPoint context)
{
    // TODO: Option to trace primary ray?

    // TODO: Option to re-use context in core PT instead of primary ray

    HitInfo hitInfo;
    bool isMiss;
    float depthNonLinear;
    ReconstructPrimaryRayHit(
        gSettings.CameraPositionWorld,
        pixelCoord,
        gGBufferMaterialIdx,
        gGBufferNormalsUv,
        gGBufferDepth,
        gMegaBufferMaterials,
        hitInfo,
        isMiss,
        depthNonLinear
    );

    RngInfo rngInfo = InitializeRngInfo(pixelCoord, 0, gSettings.FrameIdx, gSettings.Seed);
    RngValuesPrimary rngPrimary = SampleRngPrimary(rngInfo);

    float3 rayOrigin;
    float3 rayDirection;
    GetPrimaryRay(
        rngPrimary, gSettings.CameraPositionWorld, gSettings.TexelSize,
        pixelCoord, gSettings.InvV, gSettings.InvP,
        gSettings.DofFocalDist, gSettings.DofLensRadius,
        rayOrigin, rayDirection);

    context.IsMiss = isMiss;
    context.DepthNonLinear = depthNonLinear;
    context.CenterRayDirection = rayDirection;

    if (!isMiss)
        context.HitInfo = hitInfo;
}

template <typename TReservoir>
void GenerateCandidates(RWStructuredBuffer< TReservoir > buffer, uint2 pixelCoord, uint seedDecorrelator)
{
    uint contextIdx = GetContextBufferIndex(pixelCoord);
    ContextSSPoint context = gContextsPrimaryRay[contextIdx];
    if (context.IsMiss)
        return;

    RngInfo rngInfo = InitializeRngInfo(pixelCoord, 0, gSettings.FrameIdx, gSettings.Seed);
    DecorrelateRng(rngInfo, seedDecorrelator);
    if (seedDecorrelator == 0)
        SampleRngPrimary(rngInfo);

    TReservoir reservoir = INIT0(TReservoir);
    WRS(rngInfo, reservoir, pixelCoord, gRestirSettings.GenCandidatesCount, context);

    uint reservoirIdxOut = GetReservoirBufferIndex_InitialCandidatesOutput(pixelCoord);
    buffer[reservoirIdxOut] = reservoir;
}

template <typename TReservoir>
void TemporalReuse(RWStructuredBuffer< TReservoir > buffer, uint2 pixelCoord, uint seedDecorrelator)
{
    uint contextIdx = GetContextBufferIndex(pixelCoord);
    ContextSSPoint context = gContextsPrimaryRay[contextIdx];

    uint reservoirIdxIn = GetReservoirBufferIndex_TemporalInput(pixelCoord);
    uint reservoirIdxOut = GetReservoirBufferIndex_TemporalOutput(pixelCoord);

    TReservoir reservoir = buffer[reservoirIdxIn];

    float2 mv = gGBufferMv[pixelCoord].xy;

    bool prevWasOffscreen = any(mv >= MOTION_VECTOR_OFFSCREEN);
    if (context.IsMiss || prevWasOffscreen)
    {
        buffer[reservoirIdxOut] = reservoir;
        TryPreserveReservoirTemporal(buffer, reservoir, pixelCoord);
        return;
    }

	RngInfo rngInfo = InitializeRngInfo(pixelCoord, 0, gSettings.FrameIdx, gSettings.Seed);
    DecorrelateRng(rngInfo, seedDecorrelator);

    bool reservoirIsEmpty = reservoir.W_Y <= 0.0f;

    int2 prevPixelCoord;
	TReservoir reservoirPrev;
	if (!TryFindTemporalReservoir(rngInfo, buffer, reservoirIsEmpty, context, reservoir.PreCandidate, pixelCoord, mv, prevPixelCoord, reservoirPrev))
	{
		buffer[reservoirIdxOut] = reservoir;
        TryPreserveReservoirTemporal(buffer, reservoir, pixelCoord);
        return;
	}

    uint contextIdxOther = GetContextBufferIndex_Prev(prevPixelCoord);
    ContextSSPoint contextOther = gContextsPrimaryRay[contextIdxOther];

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
        Shift(reservoirPrev.Y, context);

        ReservoirMerge(rngInfo, gRestirSettings.TemporalConfidenceCap, reservoir, reservoirPrev, context);

        SetUCW(reservoir);
    }

    buffer[reservoirIdxOut] = reservoir;
    TryPreserveReservoirTemporal(buffer, reservoir, pixelCoord);
}

template <typename TReservoir>
void SpatialReuse(RWStructuredBuffer< TReservoir > buffer, uint2 pixelCoord, uint seedDecorrelator)
{
    uint reservoirIdxIn = GetReservoirBufferIndex_SpatialInput(pixelCoord);
    uint reservoirIdxOut = GetReservoirBufferIndex_SpatialOutput(pixelCoord);
    TReservoir reservoir = buffer[reservoirIdxIn];

    uint contextIdx = GetContextBufferIndex(pixelCoord);
    ContextSSPoint context = gContextsPrimaryRay[contextIdx];
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

        ContextSSPoint contextOther;
        {
            uint contextIdxOther = GetContextBufferIndex(coord);
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
        TReservoir reservoirNeighbour = buffer[idx];

        if (reservoirNeighbour.W_Y <= 0.0f || reservoirNeighbour.Confidence <= 0.0f)
            continue;

        Shift(reservoirNeighbour.Y, context);
        ReservoirMerge(rngInfo, gRestirSettings.SpatialConfidenceCap, reservoir, reservoirNeighbour, context);
    }

    SetUCW(reservoir);

    buffer[reservoirIdxOut] = reservoir;
    TryPreserveReservoirSpatial(buffer, reservoir, pixelCoord);
}

#endif