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

    uint reservoirIdxIn = GetReservoirBufferIndex_TemporalInput(pixelCoord);
    uint reservoirIdxOut = GetReservoirBufferIndex_TemporalOutput(pixelCoord);
    ReservoirDI reservoir = gReservoirBuffer[reservoirIdxIn];

    // TODO: MVs

    uint reservoirIdxPrev = GetReservoirBufferIndex_Previous(pixelCoord);
    ReservoirDI reservoirPrev = gReservoirBuffer[reservoirIdxPrev];

    bool isValidPrev = true;
    isValidPrev &= reservoirPrev.W_Y > 0.0f;
    isValidPrev &= reservoirPrev.PHat > 0.0f;
    isValidPrev &= reservoirPrev.Confidence > 0.0f;
    isValidPrev &= dot(reservoir.Info.HitInfo.Ns_ff, reservoirPrev.Info.HitInfo.Ns_ff) >= gSettings.RestirTemporalNormalDivThreshold;
    // TODO: Depth rejection
    // TODO: Occlusion rejection

    // Note: ReEvaluate needed if animation added

    // TODO: Something wrong with VisibilityTest, causes bias

    // TODO: When MVs added, lack of ping-pong could cause memory collision issues

    if (isValidPrev)
    {
        RngInfo rngInfo = InitializeRngInfo(pixelCoord, 0, gSettings.FrameIdx, gSettings.Seed);
        DecorrelateRng(rngInfo, SEED_DECORRELATOR_TEMPORAL);

        ReservoirMerge<SubReservoir_DI, SubReservoirInfo_DI>(rngInfo, gSettings.RestirTemporalConfidenceCap, reservoir, reservoirPrev);

        if (reservoir.Confidence <= 0.0f || reservoir.PHat <= 0.0f)
        {
            reservoir.Confidence = 0.0f;
            reservoir.W_Y = 0.0f;
        }
        else
            reservoir.W_Y = reservoir.WeightSum / (reservoir.PHat * reservoir.Confidence);
    }

    gReservoirBuffer[reservoirIdxOut] = reservoir;
    gReservoirBuffer[reservoirIdxPrev] = reservoir;
}
