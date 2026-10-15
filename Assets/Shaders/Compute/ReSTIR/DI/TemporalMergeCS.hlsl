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

    if (isValid)
    {
        ReservoirDI reservoirNew = CreateReservoir<SubReservoir_DI, SubReservoirInfo_DI>();

        RngInfo rngInfo = InitializeRngInfo(pixelCoord, 0, gSettings.FrameIdx, gSettings.Seed);
        DecorrelateRng(rngInfo, SEED_DECORRELATOR_TEMPORAL);

        float wCurr = reservoir.PHat * reservoir.W_Y * reservoir.Confidence;
        ReservoirUpdate<SubReservoir_DI, SubReservoirInfo_DI>(rngInfo, reservoirNew, reservoir.Y, wCurr, reservoir.Confidence, reservoir.PHat);

        reservoirPrev.PHat = Target(reservoirPrev.Y, reservoir.Info);
        float wPrev = reservoirPrev.PHat * reservoirPrev.W_Y * reservoirPrev.Confidence;
        ReservoirUpdate<SubReservoir_DI, SubReservoirInfo_DI>(rngInfo, reservoirNew, reservoirPrev.Y, wPrev, reservoirPrev.Confidence, reservoirPrev.PHat);

        reservoirNew.W_Y = reservoirNew.WeightSum / (reservoirNew.PHat * reservoirNew.Confidence);

        reservoir = reservoirNew;
    }

    gReservoirBuffer[reservoirIdx] = reservoir;
    gReservoirBuffer[reservoirIdxPrev] = reservoir;

    return;
/*

    float pHatPrev = reservoirPrev.PHat;
    reservoirPrev.PHat = Target(reservoirPrev.Y, reservoir.Info);
    float pdfPrev = PDF(reservoirPrev.Y, reservoir.Info);

    if (pdfPrev <= 0.0f || pHatPrev <= 0.0f || reservoirPrev.Confidence <= 0.0f)
    {
        gReservoirBuffer[reservoirIdx] = reservoir;
        gReservoirBuffer[reservoirIdxPrev] = reservoir;
        return;
    }

    //float ConfPrev = reservoirPrev.Confidence;
    //reservoirPrev.Confidence = min(reservoirPrev.Confidence, (float)gSettings.RestirTemporalConfidenceCap);
    //float MCapRatio = reservoirPrev.Confidence / ConfPrev;
    reservoirPrev.Confidence = min(reservoirPrev.Confidence, reservoir.Confidence * gSettings.RestirTemporalConfidenceCap);

    float W_X_i = 1.0f / pdfPrev;
    float w = reservoirPrev.PHat * reservoirPrev.WeightSum / pHatPrev;

    ReservoirMerge(rngInfo, gSettings.RestirTemporalConfidenceCap, reservoir, reservoirPrev, w);

    gReservoirBuffer[reservoirIdx] = reservoir;
    gReservoirBuffer[reservoirIdxPrev] = reservoir;
*/
}
