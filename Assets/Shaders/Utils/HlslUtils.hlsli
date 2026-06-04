#ifndef H_MATH_UTILS_H
#define H_MATH_UTILS_H

// https://sakibsaikia.github.io/graphics/2022/01/04/Nan-Checks-In-HLSL.html
bool IsNaN(float x) // WARNING: This may be giving false positives? See mul(cMatXyzToRgb, float3(0.04491435,4.6650298,2.231335))
{
    return (asuint(x) & 0x7fffffff) > 0x7f800000;
}

bool IsNaN3(float3 x)
{
    return IsNaN(x.x) || IsNaN(x.y) || IsNaN(x.z);
}

bool IsInf(float x)
{
    return !IsNaN(x) && IsNaN(x * 0.0);
}

bool IsInf3(float3 x)
{
    return IsInf(x.x) || IsInf(x.y) || IsInf(x.z);
}

float3 DebugInfoColor(float x)
{
    return IsInf(x) ? float3(0, 1, 1) : (IsNaN(x) ? float3(1, 0, 1) : x.xxx);
}

float3 DebugInfoColor(float3 x)
{
    return IsInf3(x) ? float3(0, 1, 1) : (IsNaN3(x) ? float3(1, 0, 1) : x);
}

float3 GetNaNVisualizerColor(uint2 pixelCoord)
{
    return float3(1, pixelCoord.x + pixelCoord.y % 2, 1);
}

float3 GetInfVisualizerColor(uint2 pixelCoord)
{
    return float3(1, pixelCoord.x + pixelCoord.y % 2, 0);
}

float3 NormalizeSafe(float3 v, float3 fallback)
{
    float len2 = dot(v, v);

    if (len2 <= 1e-20f || !isfinite(len2))
        return fallback;

    return v * rsqrt(len2);
}

bool CheckTIR(float n1, float n2, float cosTi)
{
    float sin2Ti = 1.0f - cosTi * cosTi;
    float sinTi = sqrt(max(0.0f, sin2Ti));
    return (n1 > n2) && (sinTi > n2 / n1);
}

float3 Reflect(float3 wo, float3 N)
{
    return wo - 2 * dot(wo, N) * N;
}

// https://www.shadertoy.com/view/7sKSRh
float erf_fast(float x) {
    return sign(x) * sqrt(1.0 - exp2(-1.787776 * x * x));
}

// https://gpuopen.com/download/Accurate_Diffuse_Lighting_from_Spherical_Gaussian_Lights_(supplemental).pdf
float erf(float x)
{
    // Early return for large |x|.
    if (abs(x) >= 4.0)
    {
        return float((int(x) & 0x80000000) ^ int(1));
    }

    // Polynomial approximation based on https://forums.developer.nvidia.com/t/optimized -version -of-single -precision -error -function -erff/40977
    if (abs(x) > 1.0)
    {
        float A1 = 1.628459513;
        float A2 = 9.15674746e-1;
        float A3 = 1.54329389e-1;
        float A4 = -3.51759829e-2;
        float A5 = 5.66795561e-3;
        float A6 = -5.64874616e-4;
        float A7 = 2.58907676e-5;
        float a = abs(x);
        float y = 1.0 - exp2 ( -((((((( A7 * a + A6) * a + A5) * a + A4) * a + A3) * a + A2) * a + A1) * a));
        return float (( int(x) & 0x80000000 ) ^ int(y));
    }
    else
    {
        float A1 = 1.128379121;
        float A2 = -3.76123011e-1;
        float A3 = 1.12799220e-1;
        float A4 = -2.67030653e-2;
        float A5 = 4.90735564e-3;
        float A6 = -5.58853149e-4;
        float x2 = x * x;
        return ((((( A6 * x2 + A5) * x2 + A4) * x2 + A3) * x2 + A2) * x2 + A1) * x;
    }
}

float cot(float x)
{
    return cos(x) / sin(x);
}

bool Approx(float2 a, float2 b, float epsilon = 1e-5f)
{
    float2 d = a - b;
    return dot(d, d) <= epsilon * epsilon;
}

// https://github.com/geometrian/simple-spectral/blob/master/src/util/color.hpp
// More exact gamma correction, less error
float3 LRGB_to_SRGB(float3 lrgb)
{
    float3 low  = 12.92 * lrgb;
    float3 high = 1.055 * pow(lrgb, 1.0 / 2.4) - 0.055;

    return float3(
        lrgb.r < 0.0031308 ? low.r  : high.r,
        lrgb.g < 0.0031308 ? low.g  : high.g,
        lrgb.b < 0.0031308 ? low.b  : high.b
    );
}

float3 SRGB_to_LRGB(float3 srgb)
{
    float3 low  = srgb / 12.92;
    float3 high = pow((srgb + 0.055) / 1.055, 2.4);

    return float3(
        srgb.r < 0.04045 ? low.r  : high.r,
        srgb.g < 0.04045 ? low.g  : high.g,
        srgb.b < 0.04045 ? low.b  : high.b
    );
}

float  RemapUtoS(float v)  { return (v - 1.0f) * 2.0f; }
float2 RemapUtoS(float2 v) { return (v - 1.0f) * 2.0f; }
float3 RemapUtoS(float3 v) { return (v - 1.0f) * 2.0f; }
float4 RemapUtoS(float4 v) { return (v - 1.0f) * 2.0f; }

float  RemapStoU(float v)  { return (v + 1.0f) * 0.5f; }
float2 RemapStoU(float2 v) { return (v + 1.0f) * 0.5f; }
float3 RemapStoU(float3 v) { return (v + 1.0f) * 0.5f; }
float4 RemapStoU(float4 v) { return (v + 1.0f) * 0.5f; }

#endif