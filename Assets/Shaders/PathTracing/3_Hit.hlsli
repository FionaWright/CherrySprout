#ifndef H_HIT_H
#define H_HIT_H

#include "PathTracing/4_GetHitInfo.hlsli"
#include "PathTracing/Debug/OutputColor.h"

#include "BxDFs/GetBxDF.hlsli"

#include "Utils/RandomDirection.h"

void Hit(inout RayQuery<RAY_FLAGS> q, inout RayDesc ray, inout float3 L_sample, inout float3 beta, inout RngInfo rngInfo, out float3 dbgOutput)
{
    HitInfo hitInfo;
    GetHitInfo(q, hitInfo);

    float3 wo = -ray.Direction;

    L_sample = beta * hitInfo.Li;

    float3 wi;
    float3 f;
    float pdf;

    BxDF bxdf;
    bxdf.Sample(rngInfo, hitInfo, wo, wi, f, pdf);

    beta *= f;

    //if (!cDebugInfoOutputEnabled && beta.x <= 0 && beta.y <= 0 && beta.z <= 0)
    //    break;

    DBG_OUTPUT_START();
    DBG_OUTPUT3(hitInfo.Ns_ff,             NormalShadedFF);
    DBG_OUTPUT3(hitInfo.Ng_ff,             NormalGeometricFF);
    DBG_OUTPUT3(hitInfo.Mat.Albedo.rgb,    Albedo);
    DBG_OUTPUT3(hitInfo.Li,                Emission);
    DBG_OUTPUT2(hitInfo.UV,                UV);
    // TODO DBG_OUTPUT_END() which does color remapping and stuff

    float3 hitPos = ray.Origin + ray.Direction * hitInfo.RayT;

    ray.Direction = wi;
    ray.Origin = hitPos + hitInfo.Ng_ff * EPSILON;
}

#endif