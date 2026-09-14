#ifndef H_GET_RAY_HIT_H
#define H_GET_RAY_HIT_H

#include "PathTracing/HitInfo/ReconstructPrimaryRay.hlsli"

void ComputeRayHit(RayQuery<RAY_FLAGS> q, uint2 pixelCoord, inout PathState pathState, out bool isMiss, out HitInfo hitInfo)
{
    if (FEATURE_ENABLED(ReconstructPrimaryRay) && pathState.RaySegmentIdx == 0)
    {
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
        return;
    }

    q.TraceRayInline(gTLAS, RAY_FLAGS, 0xFF, pathState.Desc);
    q.Proceed();

    isMiss = q.CommittedStatus() != COMMITTED_TRIANGLE_HIT;

    if (!isMiss)
        GetHitInfo(q, hitInfo);
}

#endif