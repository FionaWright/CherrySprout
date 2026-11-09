#include "Utils/SharedUtils.h"
#include "Utils/CBVs.h"
#include "PathTracing/Structs.h"

#include "PathTracing/Buffers.hlsli"
#include "PathTracing/ReSTIR/PT.hlsli"

#include "PathTracing/ReSTIR/ReservoirBuffer.hlsli"
#include "PathTracing/ReSTIR/WRS.hlsli"
#include "Compute/ReSTIR/SeedDecorrelators.h"

[numthreads(16,16,1)]
void CSMain(uint3 DTid : SV_DispatchThreadID)
{
    uint2 pixelCoord = DTid.xy;

    if (pixelCoord.x >= gSettings.FrameDimensions.x || pixelCoord.y >= gSettings.FrameDimensions.y)
        return;

    DBG_SET_PIXEL_INFO(pixelCoord, gSettings.FrameIdx);
    DBG_PATH_DUMP_CLEAR();

    uint reservoirIdxOut = GetReservoirBufferIndex_InitialCandidatesOutput(pixelCoord);
    ReservoirPT reservoir = CreateReservoir<CandidatePT>();

    uint contextIdx = GetContextBufferIndex(pixelCoord);
    ContextPrimaryRay context = gContextsPrimaryRay[contextIdx];
    if (context.IsMiss)
    {
        gReservoirBufferPT[reservoirIdxOut] = reservoir;
        return;
    }

    RngInfo rngInfo = InitializeRngInfo(pixelCoord, 0, gSettings.FrameIdx, gSettings.Seed);
    //DecorrelateRng(rngInfo, SEED_DECORRELATOR_PT_GEN_SAMPLES);
    RngValuesPrimary _ = SampleRngPrimary(rngInfo);

    WRS(rngInfo, reservoir, gRestirSettings.GenCandidatesCount, context);

    gReservoirBufferPT[reservoirIdxOut] = reservoir;
}
