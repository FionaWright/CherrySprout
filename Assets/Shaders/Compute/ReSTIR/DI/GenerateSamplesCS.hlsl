#include "Utils/SharedUtils.h"
#include "Utils/CBVs.h"

#include "PathTracing/ReSTIR/ReSTIR_DI.h"

RWStructuredBuffer<ReservoirDI> gReservoirBuffer : register(u0);

ConstantBuffer<CbvPathTracingSettings> gSettings : register(b0);
ConstantBuffer<CbvRestirSettings> gRestirSettings : register(b1);

#define SEED_DECORRELATOR 0x46362346 // TODO

[numthreads(16,16,1)]
void CSMain(uint3 DTid : SV_DispatchThreadID)
{
    uint2 pixelCoord = DTid.xy;

    if (pixelCoord.x > gSettings.Dimensions.x || pixelCoord.y > gSettings.Dimensions.y)
        return;

    uint reservoirIdx = GetReservoirBufferIndex_Current(pixelCoord);

    RngInfo rngInfo = InitializeRngInfo(pixelCoord, 0, gSettings.FrameIdx);
    rngInfo.IndependentRngState ^= SEED_DECORRELATOR;

    HitInfo hitInfo;
    bool isMiss;
    ConstructFirstHitFromGBuffer(gSettings.CameraPositionWorld, pixelCoord, hitInfo, isMiss);

    ReservoirDI reservoir = CreateReservoir();

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
    info.HitPosOffset = hitPos + hitInfo.Ns_ff * EPSILON;

    WRS(rngInfo, reservoir, gRestirSettings.NumCandidates, gRestirSettings.ConfidenceCap, info);

    gReservoirBuffer[reservoirIdx] = reservoir;
}
