#ifndef H_GAMMA_CORRECT_H
#define H_GAMMA_CORRECT_H

// https://github.com/geometrian/simple-spectral/blob/master/src/util/color.hpp

float3 LRGB_to_SRGB_Exact(float3 lrgb)
{
    float3 low  = 12.92 * lrgb;
    float3 high = 1.055 * pow(lrgb, 1.0 / 2.4) - 0.055;

    return float3(
        lrgb.r < 0.0031308 ? low.r  : high.r,
        lrgb.g < 0.0031308 ? low.g  : high.g,
        lrgb.b < 0.0031308 ? low.b  : high.b
    );
}

float3 SRGB_to_LRGB_Exact(float3 srgb)
{
    float3 low  = srgb / 12.92;
    float3 high = pow((srgb + 0.055) / 1.055, 2.4);

    return float3(
        srgb.r < 0.04045 ? low.r  : high.r,
        srgb.g < 0.04045 ? low.g  : high.g,
        srgb.b < 0.04045 ? low.b  : high.b
    );
}

float3 LRGB_to_SRGB_Fast(float3 lrgb)
{
    return pow(lrgb, 1.0f/2.2f);
}

float3 SRGB_to_LRGB_Fast(float3 lrgb)
{
    return pow(lrgb, 2.2f);
}

#endif