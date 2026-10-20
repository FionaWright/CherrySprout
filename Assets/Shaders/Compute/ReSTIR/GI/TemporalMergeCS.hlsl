#include "Utils/SharedUtils.h"
#include "Utils/CBVs.h"
#include "PathTracing/Structs.h"

#include "PathTracing/Buffers.hlsli"
#include "PathTracing/ReSTIR/GI.hlsli"

#include "Compute/ReSTIR/SeedDecorrelators.h"
#include "Compute/ReSTIR/Thresholds.hlsli"

[numthreads(16,16,1)]
void CSMain(uint3 DTid : SV_DispatchThreadID)
{
    uint2 pixelCoord = DTid.xy;

    if (pixelCoord.x >= gSettings.FrameDimensions.x || pixelCoord.y >= gSettings.FrameDimensions.y)
        return;

    uint contextIdxMain = GetContextBufferIndex(pixelCoord);
    uint contextIdxMainPrev = GetContextBufferIndex_Prev(pixelCoord);
    ContextPrimaryRay context = gContextsPrimaryRay[contextIdxMain];

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

    uint contextIdxOther = GetContextBufferIndex_Prev(prevPixelCoord);
    ContextPrimaryRay contextOther = gContextsPrimaryRay[contextIdxOther];

    float3 normalMain = context.HitInfo.Ns_ff;
    float3 normalOther = contextOther.HitInfo.Ns_ff;

    float depthMain = LinearizeDepth(context.DepthNonLinear, gSettings.NearPlane, gSettings.FarPlane);
    float depthOther = LinearizeDepth(contextOther.DepthNonLinear, gSettings.NearPlane, gSettings.FarPlane);

    bool isValidPrev = true;
    isValidPrev &= !contextOther.IsMiss;
    isValidPrev &= reservoirPrev.W_Y > 0.0f;
    isValidPrev &= reservoirPrev.PHat > 0.0f;
    isValidPrev &= reservoirPrev.Confidence > 0.0f;
    isValidPrev &= PassNormalThreshold(normalMain, normalOther, gSettings.RestirTemporalNormalDivThreshold);
    isValidPrev &= PassDepthThreshold(depthMain, depthOther, gSettings.RestirTemporalDepthDivThreshold);

    // TODO: When MVs added, lack of ping-pong could cause memory collision issues

    if (isValidPrev)
    {
        RngInfo rngInfo = InitializeRngInfo(pixelCoord, 0, gSettings.FrameIdx, gSettings.Seed);
        DecorrelateRng(rngInfo, SEED_DECORRELATOR_GI_TEMPORAL);

        ReEvaluate(reservoirPrev.Y, context);

        ReservoirMerge(rngInfo, gSettings.RestirTemporalConfidenceCap, reservoir, reservoirPrev, context);

        NormalizeUnbiasedContributionWeight(reservoir);
    }

    gReservoirBufferDI[reservoirIdxOut] = reservoir;
    gReservoirBufferDI[reservoirIdxPrevOut] = reservoir;
    gContextsPrimaryRay[contextIdxMainPrev] = context; // Optimization: Could use ping-pong alternating on frames
}
