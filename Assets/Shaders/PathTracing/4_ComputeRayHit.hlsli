#ifndef H_GET_RAY_HIT_H
#define H_GET_RAY_HIT_H

#include "PathTracing/HitInfo/ReconstructPrimaryRay.hlsli"

void ComputeRayHit(uint2 pixelCoord, inout PathState pathState, out bool isMiss, out HitInfo hitInfo)
{
    if (FEAT_CORE(ReconstructPrimaryRay) && pathState.GetPathFlag(eIsPrimaryRay))
    {
        float _;
        ReconstructPrimaryRayHit(
            gSettings.CameraPositionWorld,
            pixelCoord,
            gGBufferMaterialIdx,
            gGBufferNormalsUv,
            gGBufferDepth,
            gMegaBufferMaterials,
            hitInfo,
            isMiss,
            _
        );
        return;
    }

    RayQuery<RAY_FLAGS> q;
    q.TraceRayInline(gTLAS, RAY_FLAGS, 0xFF, pathState.Desc);
    while (q.Proceed()) {}

    isMiss = q.CommittedStatus() != COMMITTED_TRIANGLE_HIT;

    if (!isMiss)
        GetHitInfo(q, hitInfo);
}

#endif