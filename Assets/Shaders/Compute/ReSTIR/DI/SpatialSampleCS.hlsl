#include "Utils/SharedUtils.h"
#include "Utils/CBVs.h"
#include "PathTracing/Structs.h"

#include "PathTracing/Buffers.hlsli"
#include "PathTracing/ReSTIR/DI.hlsli"

#include "Compute/ReSTIR/SeedDecorrelators.h"

int2 SampleDisc(inout RngInfo rngInfo, int2 pixelCoord, float radius)
{
    float u1 = Rand01(rngInfo);
    float u2 = Rand01(rngInfo);

    float r = radius * sqrt(u1);
    float theta = 2.0f * PI * u2;

    int2 offset = int2(round(r * float2(cos(theta), sin(theta))));
    return pixelCoord + offset;
}

[numthreads(16,16,1)]
void CSMain(uint3 DTid : SV_DispatchThreadID)
{
    uint2 pixelCoord = DTid.xy;

    if (pixelCoord.x >= gSettings.FrameDimensions.x || pixelCoord.y >= gSettings.FrameDimensions.y)
        return;

    uint reservoirIdxIn = GetReservoirBufferIndex_SpatialInput(pixelCoord);
    uint reservoirIdxOut = GetReservoirBufferIndex_SpatialOutput(pixelCoord);
    ReservoirDI reservoir = gReservoirBuffer[reservoirIdxIn];

    // TODO: Occlusion rejection

    // TODO: Set up path-dumper, asserts and output color systems into restir pre-passes. Basically just requires passing a little state that can be resolved on the PT side

    float3 N_main = reservoir.Context.HitInfo.Ns_ff;
    float depth_main = gGBufferDepth[pixelCoord].x;

    RngInfo rngInfo = InitializeRngInfo(pixelCoord, 0, gSettings.FrameIdx, gSettings.Seed);
    DecorrelateRng(rngInfo, SEED_DECORRELATOR_SPATIAL);

    for (uint i = 0; i < gSettings.RestirSpatialNumSamples; i++)
    {
        int2 coord = SampleDisc(rngInfo, pixelCoord, gSettings.RestirSpatialRadius);
        //int2 coord = pixelCoord + int2(i+1,2*i+1);

        //float2 offset = RemapUtoS(float2(Rand01(rngInfo), Rand01(rngInfo)));
        //int2 coord = int2(pixelCoord) + offset * gSettings.RestirSpatialRadius;

        if (all(coord == pixelCoord)) // TODO: Is this allowed?
            continue;

        coord.x = max(0, min(coord.x, gSettings.FrameDimensions.x - 1));
        coord.y = max(0, min(coord.y, gSettings.FrameDimensions.y - 1));

        uint idx = GetReservoirBufferIndex_SpatialInput(coord);
        ReservoirDI reservoirNeighbour = gReservoirBuffer[idx];

        float3 N_neighbour = reservoirNeighbour.Context.HitInfo.Ns_ff; // TODO: This is fine even though it's not technically from gbuffer right?
        if (dot(N_main, N_neighbour) < gSettings.RestirSpatialNormalDivThreshold)
            continue;

        float depth_neighbour = gGBufferDepth[coord].x; // TODO: Convert to linear
        if (depth_main / depth_neighbour < 1.0f - gSettings.RestirSpatialDepthDivThreshold ||
            depth_neighbour / depth_main > 1.0f + gSettings.RestirSpatialDepthDivThreshold)
            continue;

        ReEvaluate(reservoirNeighbour.Y, reservoir.Context);

        ReservoirMerge<CandidateDI, ContextDI>(rngInfo, gSettings.RestirSpatialConfidenceCap, reservoir, reservoirNeighbour);

        if (reservoir.Confidence <= 0.0f || reservoir.PHat <= 0.0f)
        {
            reservoir.Confidence = 0.0f;
            reservoir.W_Y = 0.0f;
        }
        else
            reservoir.W_Y = reservoir.WeightSum / (reservoir.PHat * reservoir.Confidence); // TODO: Recompute between samples?
    }

    gReservoirBuffer[reservoirIdxOut] = reservoir;
}
