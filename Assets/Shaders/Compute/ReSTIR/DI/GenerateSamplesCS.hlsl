#include "Utils/SharedUtils.h"
#include "Utils/CBVs.h"
#include "PathTracing/Structs.h"

#include "PathTracing/Buffers.hlsli"
#include "PathTracing/ReSTIR/DI.hlsli"
#include "PathTracing/3_GetPrimaryRay.hlsli"
#include "PathTracing/HitInfo/ReconstructPrimaryRay.hlsli"

#include "Compute/ReSTIR/SeedDecorrelators.h"

[numthreads(16,16,1)]
void CSMain(uint3 DTid : SV_DispatchThreadID)
{
    uint2 pixelCoord = DTid.xy;

    if (pixelCoord.x >= gSettings.FrameDimensions.x || pixelCoord.y >= gSettings.FrameDimensions.y)
        return;

    uint reservoirIdxOut = GetReservoirBufferIndex_InitialCandidatesOutput(pixelCoord);
    uint contextIdx = GetReservoirBufferIndexLocal(pixelCoord);

    RngInfo rngInfo = InitializeRngInfo(pixelCoord, 0, gSettings.FrameIdx, gSettings.Seed);
    DecorrelateRng(rngInfo, SEED_DECORRELATOR_GEN_SAMPLES);

    // TODO: Jitter GBuffers. Requires per-frame jitter option instead of per-pixel

    HitInfo hitInfo;
    bool isMiss;
    ReconstructPrimaryRayHit(
        gSettings.CameraPositionWorld,
        pixelCoord,
        gGBufferMaterialIdx,
        gGBufferNormalsUv,
        gGBufferDepth,
        gMegaBufferMaterials,
        hitInfo,
        isMiss
    );

    ReservoirDI reservoir = CreateReservoir<CandidateDI>();
    ContextPrimaryRay context = (ContextPrimaryRay)0;

    context.IsMiss = isMiss;

    if (isMiss)
    {
        gReservoirBufferDI[reservoirIdxOut] = reservoir;
        gContextsPrimaryRay[contextIdx] = context;
        return;
    }

    float3 rayOrigin;
    float3 rayDirection;
    GetPrimaryRay(
        rngInfo, gSettings.CameraPositionWorld, gSettings.TexelSize,
        pixelCoord, gSettings.InvV, gSettings.InvP,
        gSettings.DofFocalDist, gSettings.DofLensRadius,
        rayOrigin, rayDirection);

    context.HitInfo = hitInfo;
    context.Wo = -rayDirection;
    gContextsPrimaryRay[contextIdx] = context;

    WRS(rngInfo, reservoir, gSettings.RestirGenCandidatesCount, context);

    gReservoirBufferDI[reservoirIdxOut] = reservoir;
}
