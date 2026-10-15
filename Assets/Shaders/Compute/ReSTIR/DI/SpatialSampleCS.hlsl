#include "Utils/SharedUtils.h"
#include "Utils/CBVs.h"
#include "PathTracing/Structs.h"

#include "PathTracing/Buffers.hlsli"
#include "PathTracing/ReSTIR/ReSTIR_DI.hlsli"

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

    uint reservoirIdx = GetReservoirBufferIndex_Current(pixelCoord);
    ReservoirDI reservoir = gReservoirBuffer[reservoirIdx];

    // TODO: Depth rejection
    // TODO: Occlusion rejection

    float3 N_main = reservoir.Info.HitInfo.Ns_ff;

    RngInfo rngInfo = InitializeRngInfo(pixelCoord, 0, gSettings.FrameIdx, gSettings.Seed);
    DecorrelateRng(rngInfo, SEED_DECORRELATOR_SPATIAL);

    for (uint i = 0; i < gSettings.RestirSpatialNumSamples; i++)
    {
        //int2 coord = SampleDisc(rngInfo, pixelCoord, gSettings.RestirSpatialRadius);
        //int2 coord = pixelCoord + int2(i+1,0);

        float2 offset = RemapUtoS(float2(Rand01(rngInfo), Rand01(rngInfo)));
        int2 coord = int2(pixelCoord) + offset * gSettings.RestirSpatialRadius;

        if (all(coord == pixelCoord)) // TODO: Is this allowed?
            continue;

        coord.x = max(0, min(coord.x, gSettings.FrameDimensions.x - 1));
        coord.y = max(0, min(coord.y, gSettings.FrameDimensions.y - 1));

        uint idx = GetReservoirBufferIndex_Current(coord);
        ReservoirDI reservoirNeighbour = gReservoirBuffer[idx];

        float3 N_neighbour = reservoirNeighbour.Info.HitInfo.Ns_ff; // TODO: This is fine even though it's not technically from gbuffer right?
        if (dot(N_main, N_neighbour) < gSettings.RestirSpatialNormalDivThreshold)
            continue;

        ReEvaluate(reservoirNeighbour.Y, reservoir.Info);

        ReservoirMerge<SubReservoir_DI, SubReservoirInfo_DI>(rngInfo, gSettings.RestirSpatialConfidenceCap, reservoir, reservoirNeighbour);

        if (reservoir.Confidence <= 0.0f || reservoir.PHat <= 0.0f)
        {
            reservoir.Confidence = 0.0f;
            reservoir.W_Y = 0.0f;
        }
        else
            reservoir.W_Y = reservoir.WeightSum / (reservoir.PHat * reservoir.Confidence); // TODO: Recompute between samples?
    }

    if (reservoir.Confidence == 0.0f)
        reservoir.Y.LightEval.Radiance = float3(1,0,0);

    gReservoirBuffer[reservoirIdx] = reservoir;
}
