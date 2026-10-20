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

    uint contextIdx = GetReservoirBufferIndexLocal(pixelCoord);
    ContextPrimaryRay context = gContextsPrimaryRay[contextIdx];
    if (context.IsMiss)
        return;

    uint reservoirIdxIn = GetReservoirBufferIndex_SpatialInput(pixelCoord);
    uint reservoirIdxOut = GetReservoirBufferIndex_SpatialOutput(pixelCoord);
    ReservoirDI reservoir = gReservoirBufferDI[reservoirIdxIn];

    // TODO: Occlusion rejection

    // TODO: Set up path-dumper, asserts and output color systems into restir pre-passes. Basically just requires passing a little state that can be resolved on the PT side

    float3 normalMain = context.HitInfo.Ns_ff;
    float depthMain = LinearizeDepth(gGBufferDepth[pixelCoord].x, gSettings.NearPlane, gSettings.FarPlane);

    RngInfo rngInfo = InitializeRngInfo(pixelCoord, 0, gSettings.FrameIdx, gSettings.Seed);
    DecorrelateRng(rngInfo, SEED_DECORRELATOR_SPATIAL);

    for (uint i = 0; i < gSettings.RestirSpatialNumSamples; i++)
    {
        int2 coord = SampleDisc(rngInfo, pixelCoord, gSettings.RestirSpatialRadius);
        coord.x = max(0, min(coord.x, gSettings.FrameDimensions.x - 1));
        coord.y = max(0, min(coord.y, gSettings.FrameDimensions.y - 1));

        if (all(coord == pixelCoord))
            continue;

        uint contextIdxOther = GetReservoirBufferIndexLocal(coord);
        ContextPrimaryRay contextOther = gContextsPrimaryRay[contextIdxOther];
        if (contextOther.IsMiss)
            continue;

        uint idx = GetReservoirBufferIndex_SpatialInput(coord);
        ReservoirDI reservoirNeighbour = gReservoirBufferDI[idx];

        float3 normalOther = contextOther.HitInfo.Ns_ff;
        if (dot(normalMain, normalOther) < gSettings.RestirSpatialNormalDivThreshold)
            continue;

        float depthOther = LinearizeDepth(gGBufferDepth[coord].x, gSettings.NearPlane, gSettings.FarPlane);
        if (depthMain / depthOther < 1.0f - gSettings.RestirSpatialDepthDivThreshold ||
            depthOther / depthMain > 1.0f + gSettings.RestirSpatialDepthDivThreshold)
            continue;

        ReEvaluate(reservoirNeighbour.Y, context);

        ReservoirMerge(rngInfo, gSettings.RestirSpatialConfidenceCap, reservoir, reservoirNeighbour, context);
    }

    NormalizeUnbiasedContributionWeight(reservoir);

    gReservoirBufferDI[reservoirIdxOut] = reservoir;
}
