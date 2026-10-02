#ifndef H_RAY_TRACED_SHADOWS_H
#define H_RAY_TRACED_SHADOWS_H

#define MAX_SHADOW_RAY_DEPTH 8
#define RAY_FLAGS RAY_FLAG_CULL_NON_OPAQUE|RAY_FLAG_SKIP_PROCEDURAL_PRIMITIVES

void TraceRayShadow(float3 pos, float3 dir, float lightDistance, out float shadowFactor)
{
    RayQuery<RAY_FLAGS> q;

    RayDesc ray;
    ray.Origin = pos;
    ray.Direction = dir;
    ray.TMin = 0.001;
    ray.TMax = lightDistance;

    shadowFactor = 1.0f;

    for (uint i = 0; i < MAX_SHADOW_RAY_DEPTH; i++)
    {
        q.TraceRayInline(gTLAS, RAY_FLAGS, 0xFF, ray);
        q.Proceed();

        bool isMiss = q.CommittedStatus() != COMMITTED_TRIANGLE_HIT;
        if (isMiss)
            return;

        shadowFactor = 0.0f;
        return;

        // TODO:

        /*

        uint instanceIdx = q.CommittedInstanceIndex();
        InstanceData instance = gMegaBufferInstanceData[instanceIdx];
        Material mat = gMegaBufferMaterials[instance.MaterialIndex];

        float2 uv = ExtractUV(q, instance);

        float4 albedoSample = mat.TexIdxAlbedo == -1 ? 1.0f : gSceneTextures[mat.TexIdxAlbedo].Sample(gSamplerLinearWrap, uv);
        float alpha = albedoSample.w;

        shadowFactor *= 1.0f - alpha;

        if (shadowFactor <= 0.0f)
            return;

        float rayT = q.CommittedRayT();
        float advance = rayT + EPSILON;
        ray.Origin += ray.Direction * advance;
        ray.TMax -= advance;

        if (ray.TMax <= 0.0f)
            return;
        */
    }
}

#endif