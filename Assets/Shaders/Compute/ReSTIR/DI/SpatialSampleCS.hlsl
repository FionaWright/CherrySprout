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

    bool isValidPrev = true;
    isValidPrev &= reservoirPrev.W_Y > 0.0f;
    isValidPrev &= reservoirPrev.PHat > 0.0f;
    isValidPrev &= reservoirPrev.Confidence > 0.0f;
    isValidPrev &= dot(reservoir.Info.HitInfo.Ns_ff, reservoirPrev.Info.HitInfo.Ns_ff) >= gSettings.RestirTemporalNormalDivThreshold;
    // TODO: Depth rejection
    // TODO: Occlusion rejection

    // Note: ReservoirPrev ReEvaluate not needed unless animation added

    if (isValidPrev)
    {
        ReservoirDI reservoirNew = CreateReservoir<SubReservoir_DI, SubReservoirInfo_DI>();
        reservoirNew.Info = reservoir.Info;

        RngInfo rngInfo = InitializeRngInfo(pixelCoord, 0, gSettings.FrameIdx, gSettings.Seed);
        DecorrelateRng(rngInfo, SEED_DECORRELATOR_TEMPORAL);

        bool candidateSelected = false;

        float wCurr = GetWeight(reservoir.PHat, reservoir.Confidence, reservoir.W_Y);
        candidateSelected |= ReservoirUpdate<SubReservoir_DI, SubReservoirInfo_DI>(rngInfo, reservoirNew, reservoir.Y, wCurr, reservoir.Confidence, reservoir.PHat);

        candidateSelected |= ReservoirMerge<SubReservoir_DI, SubReservoirInfo_DI>(rngInfo, gSettings.RestirTemporalConfidenceCap, reservoirNew, reservoirPrev);

        if (!candidateSelected || reservoirNew.Confidence <= 0.0f || reservoirNew.PHat <= 0.0f)
        {
            reservoirNew.Confidence = 0.0f;
            reservoirNew.W_Y = 0.0f;
        }
        else
            reservoirNew.W_Y = reservoirNew.WeightSum / (reservoirNew.PHat * reservoirNew.Confidence);

        reservoir = reservoirNew;
    }

    gReservoirBuffer[reservoirIdx] = reservoir;
    gReservoirBuffer[reservoirIdxPrev] = reservoir;
}
