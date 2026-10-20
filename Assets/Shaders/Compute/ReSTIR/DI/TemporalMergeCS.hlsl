#include "Utils/SharedUtils.h"
#include "Utils/CBVs.h"
#include "PathTracing/Structs.h"

#include "PathTracing/Buffers.hlsli"
#include "PathTracing/ReSTIR/DI.hlsli"

#include "Compute/ReSTIR/SeedDecorrelators.h"

[numthreads(16,16,1)]
void CSMain(uint3 DTid : SV_DispatchThreadID)
{
    uint2 pixelCoord = DTid.xy;

    if (pixelCoord.x >= gSettings.FrameDimensions.x || pixelCoord.y >= gSettings.FrameDimensions.y)
        return;

    uint contextIdx = GetReservoirBufferIndexLocal(pixelCoord);
    ContextPrimaryRay context = gContextsPrimaryRay[contextIdx];

    uint reservoirIdxIn = GetReservoirBufferIndex_TemporalInput(pixelCoord);
    uint reservoirIdxOut = GetReservoirBufferIndex_TemporalOutput(pixelCoord);
    uint reservoirIdxPrevOut = GetReservoirBufferIndex_Previous(pixelCoord);

    ReservoirDI reservoir = gReservoirBufferDI[reservoirIdxIn];

    float2 mv = gGBufferMv[pixelCoord].xy;

    bool prevWasOffscreen = mv.x >= 9.0f || mv.y >= 9.0f;
    if (context.IsMiss || prevWasOffscreen)
    {
        gReservoirBufferDI[reservoirIdxOut] = reservoir;
        gReservoirBufferDI[reservoirIdxPrevOut] = reservoir;
        return;
    }

    int2 prevPixelCoord = MotionVectorToPixelCoord(pixelCoord, mv, gSettings.FrameDimensions);

    bool validPrevPixel = all(prevPixelCoord >= 0) && all(prevPixelCoord < int2((float2)gSettings.FrameDimensions));
    if (!validPrevPixel)
    {
        gReservoirBufferDI[reservoirIdxOut] = reservoir;
        gReservoirBufferDI[reservoirIdxPrevOut] = reservoir;
        return;
    }

    uint reservoirIdxPrevIn = GetReservoirBufferIndex_Previous(prevPixelCoord);
    ReservoirDI reservoirPrev = gReservoirBufferDI[reservoirIdxPrevIn];

    //uint contextIdxOther = GetReservoirBufferIndexLocal(prevPixelCoord);
    //ContextPrimaryRay contextOther = gContextsPrimaryRay[contextIdxOther];

    //float3 normalMain = context.HitInfo.Ns_ff;
    //float depthMain = gGBufferDepth[pixelCoord].x;

    //float3 normalOther = contextOther.HitInfo.Ns_ff;
    //float depthOther = gGBufferDepth[prevPixelCoord].x;

    // TODO: Need previous context to do normal/depth thresholding

    bool isValidPrev = true;
    //isValidPrev &= !contextOther.IsMiss;
    isValidPrev &= reservoirPrev.W_Y > 0.0f;
    isValidPrev &= reservoirPrev.PHat > 0.0f;
    isValidPrev &= reservoirPrev.Confidence > 0.0f;
    //isValidPrev &= dot(normalMain, normalOther) >= gSettings.RestirTemporalNormalDivThreshold;
    // TODO: Depth
    // TODO: Occlusion rejection

    // TODO: When MVs added, lack of ping-pong could cause memory collision issues

    if (isValidPrev)
    {
        RngInfo rngInfo = InitializeRngInfo(pixelCoord, 0, gSettings.FrameIdx, gSettings.Seed);
        DecorrelateRng(rngInfo, SEED_DECORRELATOR_TEMPORAL);

        ReEvaluate(reservoirPrev.Y, context);

        ReservoirMerge(rngInfo, gSettings.RestirTemporalConfidenceCap, reservoir, reservoirPrev, context);

        if (reservoir.Confidence <= 0.0f || reservoir.PHat <= 0.0f)
        {
            reservoir.Confidence = 0.0f;
            reservoir.W_Y = 0.0f;
        }
        else
            reservoir.W_Y = reservoir.WeightSum / (reservoir.PHat * reservoir.Confidence);
    }

    gReservoirBufferDI[reservoirIdxOut] = reservoir;
    gReservoirBufferDI[reservoirIdxPrevOut] = reservoir;
}
