#ifndef H_RANDOM_EXTRAS_H
#define H_RANDOM_EXTRAS_H

#include "Utils/Random.h"
#include "Utils/Math/ShadingFrame.h"

inline hlsl::float3 RandDirectionCube(GLUE_INOUT(hlsl::uint) state)
{
    const float x = PcgRand01(state) * 2. - 1.;
    const float y = PcgRand01(state) * 2. - 1.;
    const float z = PcgRand01(state) * 2. - 1.;
    return normalize(hlsl::float3(x, y, z));
}

inline hlsl::float3 RandDirectionUniform(GLUE_INOUT(hlsl::uint) state)
{
    const float u = PcgRand01(state); // [0,1)
    const float v = PcgRand01(state); // [0,1)

    const float z = 1.0 - 2.0 * u;
    const float r = sqrt(saturate(1.0 - z * z));
    const float phi = 2.0 * PI * v;

    return hlsl::float3(r * cos(phi), r * sin(phi), z);
}

inline hlsl::float3 RandHemisphereUniformSSpace(const float u1, const float u2)
{
    const float z = u1;                 // glueCos(theta)
    const float r = sqrt(max(0.0f, 1.0f - z*z));
    const float phi = 2 * PI * u2;

    return normalize(hlsl::float3(
        r * cos(phi),
        r * sin(phi),
        z
    ));
}

inline hlsl::float3 RandHemisphereCosineSSpace(const float u1, const float u2)
{
    const float r = sqrt(u1);
    const float theta = 2.0f * PI * u2;

    const float x = r * cos(theta);
    const float y = r * sin(theta);
    const float z = sqrt(1.0f - u1); // ensures cosine weighting

    return normalize(hlsl::float3(x, y, z));
}

inline hlsl::float3 RandHemisphereCosineWorld(const float u1, const float u2, ShadingFrame frame)
{
    const hlsl::float3 L_s = RandHemisphereCosineSSpace(u1, u2);
    return frame.ToWorld(L_s);
}

inline hlsl::float3 RandHemisphereUniformWorld(const float u1, const float u2, ShadingFrame frame)
{
    const hlsl::float3 L_s = RandHemisphereUniformSSpace(u1, u2);
    return frame.ToWorld(L_s);
}

inline hlsl::int2 SampleDisc(const float u1, const float u2, const hlsl::int2 pixelCoord, const float radius)
{
    const float r = radius * sqrt(u1);
    const float theta = 2.0f * PI * u2;

    const hlsl::float2 dir = hlsl::float2(cos(theta), sin(theta));
    const float x = pixelCoord.x + int(round(r * dir.x));
    const float y = pixelCoord.y + int(round(r * dir.y));

    return hlsl::int2(x, y);
}

#endif