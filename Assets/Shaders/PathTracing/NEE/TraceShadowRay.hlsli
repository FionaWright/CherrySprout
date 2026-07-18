#ifndef H_SHADOW_RAY_H
#define H_SHADOW_RAY_H

void TraceShadowRay(float3 pos, float3 dir, out bool occluded, float lightDistance)
{
    // TODO: Does it matter to recreate the RayQuery?
    RayQuery<RAY_FLAGS> q;

    RayDesc ray;
    ray.Origin = pos;
    ray.Direction = dir;
    ray.TMin = 0.001;
    ray.TMax = 1000.0;

    // TODO: Transmission/alpha handling

    for (uint i = 0; i <= gSettings.MaxShadowRayDepth; i++)
    {
        q.TraceRayInline(gTLAS, RAY_FLAGS, 0xFF, ray);
        q.Proceed();

        bool miss = q.CommittedStatus() != COMMITTED_TRIANGLE_HIT;
        if (miss)
        {
            occluded = false;
            return;
        }
        else
        {
            occluded = q.CommittedRayT() < lightDistance;;
            return;
        }
    }

    occluded = q.CommittedRayT() < lightDistance;;
    return;
}

#endif