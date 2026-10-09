#ifndef H_PT_UTILS_H
#define H_PT_UTILS_H

#define RAY_FLAGS RAY_FLAG_CULL_NON_OPAQUE|RAY_FLAG_SKIP_PROCEDURAL_PRIMITIVES

#include "PathTracing/Flags/Internal/MethodsHlsl.hlsli"
#include "PathTracing/Debug/Internal/OutputColorMacros.hlsli"
#include "PathTracing/Structs.h"
#include "Utils/HlslUtils.hlsli"

PathState CreatePathState(float3 origin, float3 dir)
{
    PathState pathState = (PathState)0;

    pathState.LastBxdfPdf = 1.0f;
    pathState.IsPrimaryRay = true;

    if (FEATURE_ENABLED(Transient))
        pathState.RollingPathDistance = 0.0f;

    pathState.Desc.TMin = 0.001;
    pathState.Desc.TMax = 1000.0;
    pathState.Desc.Origin = origin;
    pathState.Desc.Direction = dir;

    pathState.Beta = float3(1, 1, 1);
    pathState.Lo = float3(0, 0, 0);

    return pathState;
}

float3 LRGB_to_SRGB(float3 color)
{
    if (FEATURE_ENABLED(GammaCorrectionFast))
        return LRGB_to_SRGB_Fast(color);

    return LRGB_to_SRGB_Exact(color);
}

float3 SRGB_to_LRGB(float3 color)
{
    if (FEATURE_ENABLED(GammaCorrectionFast))
        return SRGB_to_LRGB_Fast(color);

    return SRGB_to_LRGB_Exact(color);
}

float3 ApplyFireflyThreshold(float3 radiance, float fireflyThreshold)
{
    if (FEATURE_ENABLED(FireflyThreshold))
    {
        float L_lum = Luminance(radiance);
        if (L_lum > fireflyThreshold)
            radiance *= fireflyThreshold / L_lum;
    }
    return radiance;
}

#endif