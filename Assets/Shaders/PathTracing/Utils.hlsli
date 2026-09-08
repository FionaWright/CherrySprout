#ifndef H_PT_UTILS_H
#define H_PT_UTILS_H

#define RAY_FLAGS RAY_FLAG_CULL_NON_OPAQUE|RAY_FLAG_SKIP_PROCEDURAL_PRIMITIVES

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

#endif