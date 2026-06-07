#ifndef H_HIT_H
#define H_HIT_H

#include "PathTracing/4_GetHitInfo.hlsli"
#include "PathTracing/Debug/OutputColor.h"

#include "Utils/RandomExtras.h"

void Hit(inout RayQuery<RAY_FLAGS> q, inout RayDesc ray, inout float3 L_sample, inout float3 throughput, inout RngInfo rngInfo, out float3 dbgOutput)
{
    HitInfo hitInfo;
    GetHitInfo(q, hitInfo);

    float3 wo = -ray.Direction;
    float3 wi;

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

    //if (!cDebugInfoOutputEnabled && throughput.x <= 0 && throughput.y <= 0 && throughput.z <= 0)
    //    break;

    DBG_OUTPUT_START();
    DBG_OUTPUT3(hitInfo.Ns,            NormalShaded);
    DBG_OUTPUT3(hitInfo.Ns_ff,         NormalShadedFF);
    DBG_OUTPUT3(hitInfo.Ng_ff,         NormalGeometricFF);
    DBG_OUTPUT3(hitInfo.Albedo.rgb,    Albedo);
    DBG_OUTPUT3(hitInfo.Li,            Emission);
    DBG_OUTPUT2(hitInfo.UV,            UV);
    // TODO DBG_OUTPUT_END() which does color remapping and stuff

    float hitDist = q.CommittedRayT();
    float3 hitPos = ray.Origin + ray.Direction * hitDist;

    ray.Direction = wi;
    ray.Origin = hitPos + hitInfo.Ng_ff * EPSILON;
}

#endif