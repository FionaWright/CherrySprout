#include "Utils/SharedUtils.h"
#include "Utils/CBVs.h"
#include "PathTracing/Structs.h"

#include "PathTracing/Buffers.hlsli"
#include "PathTracing/ReSTIR/ReSTIR_DI.hlsli"
#include "PathTracing/3_GetPrimaryRay.hlsli"
#include "PathTracing/HitInfo/ReconstructPrimaryRay.hlsli"

#define SEED_DECORRELATOR 0x46362346 // TODO

[numthreads(16,16,1)]
void CSMain(uint3 DTid : SV_DispatchThreadID)
{
    uint2 pixelCoord = DTid.xy;

    if (pixelCoord.x > gSettings.FrameDimensions.x || pixelCoord.y > gSettings.FrameDimensions.y)
        return;

    uint reservoirIdx = GetReservoirBufferIndex_Current(pixelCoord);

    RngInfo rngInfo = InitializeRngInfo(pixelCoord, 0, gSettings.FrameIdx);
    rngInfo.IndependentRngState ^= SEED_DECORRELATOR;

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

    ReservoirDI reservoir = CreateReservoir<LightSample>();

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

    float3 hitPos = rayOrigin + rayDirection * hitInfo.RayT;

    LightSampleSelectionInfo info;
    info.Dir_wo = -rayDirection;
    info.HitInfo = hitInfo;
    info.HitPos = hitPos;
    info.HitPosOffset = hitPos + hitInfo.Ns_ff * EPSILON;

    WRS(rngInfo, reservoir, gSettings.RestirNumCandidates, gSettings.RestirConfidenceCap, info);

    gReservoirBuffer[reservoirIdx] = reservoir;
}
