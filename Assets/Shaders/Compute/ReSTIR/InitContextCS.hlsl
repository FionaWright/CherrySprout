#include "Utils/SharedUtils.h"
#include "Utils/CBVs.h"
#include "PathTracing/Structs.h"

#include "PathTracing/Buffers.hlsli"
#include "PathTracing/3_GetPrimaryRay.hlsli"
#include "PathTracing/HitInfo/ReconstructPrimaryRay.hlsli"

#include "PathTracing/ReSTIR/ReservoirBuffer.hlsli"

[numthreads(16,16,1)]
void CSMain(uint3 DTid : SV_DispatchThreadID)
{
    uint2 pixelCoord = DTid.xy;

    if (pixelCoord.x >= gSettings.FrameDimensions.x || pixelCoord.y >= gSettings.FrameDimensions.y)
        return;

    uint contextIdx = GetContextBufferIndex(pixelCoord);

    // TODO: Option to trace primary ray?

    // TODO: Option to re-use context in core PT instead of primary ray

    HitInfo hitInfo;
    bool isMiss;
    float depthNonLinear;
    ReconstructPrimaryRayHit(
        gSettings.CameraPositionWorld,
        pixelCoord,
        gGBufferMaterialIdx,
        gGBufferNormalsUv,
        gGBufferDepth,
        gMegaBufferMaterials,
        hitInfo,
        isMiss,
        depthNonLinear
    );

    ContextPrimaryRay context = (ContextPrimaryRay)0;
    context.IsMiss = isMiss;
    context.DepthNonLinear = depthNonLinear;

    if (isMiss)
    {
        gContextsPrimaryRay[contextIdx] = context;
        return;
    }

    // Note: No seed decorrelator used, will have same jitter / DoF as core PT
    RngInfo rngInfo = InitializeRngInfo(pixelCoord, 0, gSettings.FrameIdx, gSettings.Seed);
    RngValuesPrimary rngPrimary = SampleRngPrimary(rngInfo);

    float3 rayOrigin;
    float3 rayDirection;
    GetPrimaryRay(
        rngPrimary, gSettings.CameraPositionWorld, gSettings.TexelSize,
        pixelCoord, gSettings.InvV, gSettings.InvP,
        gSettings.DofFocalDist, gSettings.DofLensRadius,
        rayOrigin, rayDirection);

    context.HitInfo = hitInfo;
    context.Wo = -rayDirection;
    gContextsPrimaryRay[contextIdx] = context;
}
