#include "Utils/SharedUtils.h"
#include "Utils/CBVs.h"
#include "PathTracing/Structs.h"

#include "PathTracing/Buffers.hlsli"
#include "PathTracing/ReSTIR/ReSTIR_DI.hlsli"

#include "Compute/ReSTIR/SeedDecorrelators.h"

[numthreads(16,16,1)]
void CSMain(uint3 DTid : SV_DispatchThreadID)
{
    uint2 pixelCoord = DTid.xy;

    if (pixelCoord.x > gSettings.FrameDimensions.x || pixelCoord.y > gSettings.FrameDimensions.y)
        return;

    uint reservoirIdx = GetReservoirBufferIndex_Current(pixelCoord);
    ReservoirDI reservoir = gReservoirBuffer[reservoirIdx];

    // TODO: MVs

    uint reservoirIdxPrev = GetReservoirBufferIndex_Previous(pixelCoord);
    ReservoirDI reservoirPrev = gReservoirBuffer[reservoirIdxPrev];

    RngInfo rngInfo = InitializeRngInfo(pixelCoord, 0, gSettings.FrameIdx, gSettings.Seed);
    DecorrelateRng(rngInfo, SEED_DECORRELATOR_TEMPORAL);

    // TODO: Normal/Distance rejection

    reservoirPrev.PHat = Target(reservoirPrev.Y, reservoir.Info);
    float pdfPrev = PDF(reservoirPrev.Y, reservoir.Info);

    if (pdfPrev <= 0.0f || reservoirPrev.Confidence <= 0.0f)
    {
        gReservoirBuffer[reservoirIdx] = reservoir;
        gReservoirBuffer[reservoirIdxPrev] = reservoir;
        return;
    }

    float W_X_i = 1.0f / pdfPrev;
    float w = reservoirPrev.PHat * W_X_i / reservoirPrev.Confidence;

    ReservoirMerge(rngInfo, gSettings.RestirTemporalConfidenceCap, reservoir, reservoirPrev, w);

    gReservoirBuffer[reservoirIdx] = reservoir;
    gReservoirBuffer[reservoirIdxPrev] = reservoir;
}
