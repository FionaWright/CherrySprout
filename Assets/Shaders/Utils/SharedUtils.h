#ifndef H_MATH_UTILS_SHADER_H
#define H_MATH_UTILS_SHADER_H

#include "Constants.h"
#include "HlslGlue.h"

// Only for functions that are required by both C++ and HLSL

inline float CopySign(float mag, float sign)
{
    return sign < 0.0f ? -abs(mag) : abs(mag);
}

inline float Sign(float sign)
{
    return CopySign(1, sign);
}

inline float SafeSqrt(float x)
{
    return sqrt(max(0.0f, x));
}

inline hlsl::float3 EaSquareToSphere(hlsl::float2 uv)
{
    // Transform to [-1, 1]^2
    const float ax = 2.0f * uv.x - 1.0f;
    const float ay = 2.0f * uv.y - 1.0f;
    const float absax = abs(ax);
    const float absay = abs(ay);

    // Compute radius and angle
    const float signedDist = 1 - (absax + absay); // Signed distance to the u + v = 1 diagonal diamond
    const float d = abs(signedDist);
    const float r = 1 - d;
    const float phi = (r == 0 ? 1 : (absay - absax) / r + 1) * PI / 4;

    // Compute vector
    const float y = CopySign(1 - r * r, signedDist);
    const float cosPhi = CopySign(cos(phi), ax);
    const float sinPhi = CopySign(sin(phi), ay);
    return hlsl::float3(cosPhi * r * SafeSqrt(2 - r * r), y, sinPhi * r * SafeSqrt(2 - r * r));
}

inline hlsl::float2 EaSphereToSquare(hlsl::float3 d)
{
    float x = abs(d.x);
    float y = abs(d.y);
    float z = abs(d.z);
    float r = SafeSqrt(1 - y);
    float a = max(x, z);
    float b = min(x, z);
    b = a == 0 ? 0 : b / a;

    float phi = atan(b) * 2.0f / PI; // Can use polynomial to optimize here?
    if (x < z)
        phi = 1 - phi;

    float v = phi * r;
    float u = r - v;
    if (d.y < 0)
    {
        float t = u;
        u = 1 - v;
        v = 1 - t;
    }
    u = CopySign(u, d.x);
    v = CopySign(v, d.z);
    return hlsl::float2(0.5f * (u + 1), 0.5f * (v + 1));
}

inline hlsl::float2 PanoSphereToSquare(hlsl::float3 d)
{
    float lambda = atan2(d.z, d.x);    // [-pi,pi]
    float phi = asin(clamp(d.y, -1.0f, 1.0f));
    float u = (lambda + PI) / (2.0f * PI);
    float v = (phi + 0.5 * PI) / PI;
    u = frac(u);
    return hlsl::float2(u, v);
}

inline hlsl::float3 CubemapCubeToSphere(hlsl::uint face, hlsl::float2 uv)
{
    hlsl::float3 d;
    switch (face)
    {
    case 0: // +X
        d = hlsl::float3(1.0, -uv.y, -uv.x);
        break;
    case 1: // -X
        d = hlsl::float3(-1.0, -uv.y, uv.x);
        break;
    case 2: // +Y
        d = hlsl::float3(uv.x, 1.0, uv.y);
        break;
    case 3: // -Y
        d = hlsl::float3(uv.x, -1.0, -uv.y);
        break;
    case 4: // +Z
        d = hlsl::float3(uv.x, -uv.y, 1.0);
        break;
    case 5: // -Z
        d = hlsl::float3(-uv.x, -uv.y, -1.0);
        break;
    default:
        return hlsl::float3(0,0,0);
    }

    return normalize(d);
}

inline float Luminance(const hlsl::float3 color)
{
    return dot(color, hlsl::float3(0.2126,0.7152,0.0722));
}

// https://backend.orbit.dtu.dk/ws/files/126824972/onb_frisvad_jgt2012_v2.pdf
inline void BuildBasisFrisvad(hlsl::float3 N, GLUE_OUT(hlsl::float3) T, GLUE_OUT(hlsl::float3) B)
{
    if (N.z < -0.999999f)
    {
        T = hlsl::float3(0, -1, 0);
        B = hlsl::float3(-1, 0, 0);
        return;
    }

    float a = 1.0 / (1.0 + N.z);
    float b = -N.x * N.y * a;
    T = hlsl::float3(1.0 - N.x * N.x * a, b, -N.x);
    B = hlsl::float3(b, 1.0 - N.y * N.y * a, -N.y);
}

#endif