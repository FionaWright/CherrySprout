#ifndef H_HIT_H
#define H_HIT_H

#include "PathTracing/4_GetHitInfo.hlsli"

#include "Utils/RandomExtras.h"

void Hit(inout RayQuery<RAY_FLAGS> q, inout RayDesc ray, inout float3 Lo, inout float3 throughput, inout RngInfo rngInfo, float3 wo, float hitDist)
{
    HitInfo hitInfo;
    GetHitInfo(q, hitInfo);

    float3 wi, L_sample;

    {
        L_sample = throughput * hitInfo.Li;

        float u1 = Rand01(rngInfo);
        float u2 = Rand01(rngInfo);

        ShadingFrame sframe = CreateShadingFrame(hitInfo.Ns_ff);

        wi = RandHemisphereUniformWorld(u1, u2, sframe);
        float NdL = saturate(dot(hitInfo.Ns_ff, wi));

        float3 diffuseBrdf = hitInfo.Albedo / PI;
        float pdf = 1.0f / (2.0f * PI);
        throughput *= diffuseBrdf * NdL / max(0.001f, pdf);
    }

    L_sample = float3(0.5, 0, 0);

    //if (!cDebugInfoOutputEnabled && throughput.x <= 0 && throughput.y <= 0 && throughput.z <= 0)
    //    break;

    Lo += L_sample;

    float3 hitPos = ray.Origin + ray.Direction * hitDist;
    ray.Direction = wi;
    ray.Origin = hitPos + hitInfo.Ng_ff * EPSILON * sign(dot(hitInfo.Ng_ff, ray.Direction));
}

#endif