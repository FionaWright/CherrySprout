#ifndef H_HIT_H
#define H_HIT_H

#include "PathTracing/4_GetHitInfo.hlsli"
#include "PathTracing/Debug/OutputColor.h"

#include "BxDFs/GetBxDF.hlsli"

#include "Utils/RandomDirection.h"

void Hit(inout RayQuery<RAY_FLAGS> q, inout RayDesc ray, inout float3 L_sample, inout float3 beta, inout RngInfo rngInfo)
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

    DBG_OUTPUT3(f,                                 f);
    DBG_OUTPUT1(pdf,                               PDF);
    DBG_OUTPUT3(hitInfo.SFrame.ToLocal(wi),        L_s);
    DBG_OUTPUT3(wi,                                L_w);

    float3 hitPos = ray.Origin + ray.Direction * hitInfo.RayT;

    ray.Direction = wi;
    ray.Origin = hitPos + hitInfo.Ng_ff * EPSILON;
}

#endif