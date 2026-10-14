#include "Utils/SharedUtils.h"
#include "Utils/CBVs.h"
#include "PathTracing/Structs.h"

#include "PathTracing/Buffers.hlsli"
#include "PathTracing/ReSTIR/ReSTIR_DI.hlsli"
#include "PathTracing/3_GetPrimaryRay.hlsli"
#include "PathTracing/HitInfo/ReconstructPrimaryRay.hlsli"

#include "Compute/ReSTIR/SeedDecorrelators.h"

[numthreads(16,16,1)]
void CSMain(uint3 DTid : SV_DispatchThreadID)
{
    uint2 pixelCoord = DTid.xy;

    if (pixelCoord.x > gSettings.FrameDimensions.x || pixelCoord.y > gSettings.FrameDimensions.y)
        return;

    uint reservoirIdx = GetReservoirBufferIndex_Current(pixelCoord);

    RngInfo rngInfo = InitializeRngInfo(pixelCoord, 0, gSettings.FrameIdx, gSettings.Seed);
    DecorrelateRng(rngInfo, SEED_DECORRELATOR_GEN_SAMPLES);

    HitInfo hitInfo;
    bool isMiss;
    ReconstructPrimaryRayHit(
        gSettings.CameraPositionWorld,
        pixelCoord,
        gGBufferMaterialIdx,
        gGBufferNormals,
        gGBufferDepth,
        gGBufferUvMv,
        gMegaBufferMaterials,
        hitInfo,
        isMiss
    );

    ReservoirDI reservoir = CreateReservoir<SubReservoir_DI>();

    if (isMiss)
    {
        gReservoirBuffer[reservoirIdx] = reservoir;
        return;
    }

    float3 rayOrigin;
    float3 rayDirection;
    GetPrimaryRay(
        rngInfo, gSettings.CameraPositionWorld, gSettings.TexelSize,
        pixelCoord, gSettings.InvV, gSettings.InvP,
        gSettings.DofFocalDist, gSettings.DofLensRadius,
        rayOrigin, rayDirection);

    SubReservoirInfo_DI info;
    info.Wo = -rayDirection;
    info.HitInfo = hitInfo;

    WRS(rngInfo, reservoir, gSettings.RestirGenCandidatesCount, gSettings.RestirGenConfidenceCap, info);

    gReservoirBuffer[reservoirIdx] = reservoir;
}
