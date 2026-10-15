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

    if (pixelCoord.x >= gSettings.FrameDimensions.x || pixelCoord.y >= gSettings.FrameDimensions.y)
        return;

    uint reservoirIdx = GetReservoirBufferIndex_Current(pixelCoord);
    ReservoirDI reservoir = gReservoirBuffer[reservoirIdx];

    // TODO: MVs

    uint reservoirIdxPrev = GetReservoirBufferIndex_Previous(pixelCoord);
    ReservoirDI reservoirPrev = gReservoirBuffer[reservoirIdxPrev];

    bool isValid = reservoirPrev.W_Y > 0.0f && reservoirPrev.PHat > 0.0f && reservoirPrev.Confidence > 0.0f;
    // TODO: Normal/Distance rejection
    // TODO: Occlusion rejection

    if (isValid)
    {
        ReservoirDI reservoirNew = CreateReservoir<SubReservoir_DI, SubReservoirInfo_DI>();

        RngInfo rngInfo = InitializeRngInfo(pixelCoord, 0, gSettings.FrameIdx, gSettings.Seed);
        DecorrelateRng(rngInfo, SEED_DECORRELATOR_TEMPORAL);

        bool candidateSelected = false;

        float wCurr = GetWeight(reservoir.PHat, reservoir.Confidence, reservoir.W_Y);
        candidateSelected |= ReservoirUpdate<SubReservoir_DI, SubReservoirInfo_DI>(rngInfo, reservoirNew, reservoir.Y, wCurr, reservoir.Confidence, reservoir.PHat);

        candidateSelected |= ReservoirMerge<SubReservoir_DI, SubReservoirInfo_DI>(rngInfo, gSettings.RestirTemporalConfidenceCap, reservoirNew, reservoirPrev);

        /*if (!candidateSelected || reservoirNew.Confidence == 0.0f || reservoirNew.PHat <= 0.0f)
        {
            reservoirNew.Confidence = 0.0f;
            reservoirNew.W_Y = 0.0f;
        }
        else*/
            reservoirNew.W_Y = reservoirNew.WeightSum / (reservoirNew.PHat * reservoirNew.Confidence);

        reservoir = reservoirNew;
    }

    gReservoirBuffer[reservoirIdx] = reservoir;
    gReservoirBuffer[reservoirIdxPrev] = reservoir;
}
