#ifndef H_RANDOM_EXTRAS_H
#define H_RANDOM_EXTRAS_H

#include "Utils/Random.h"
#include "Utils/Math/ShadingFrame.h"

inline hlsl::float3 RandDirectionCube(GLUE_INOUT(hlsl::uint) state)
{
    float x = PcgRand01(state) * 2. - 1.;
    float y = PcgRand01(state) * 2. - 1.;
    float z = PcgRand01(state) * 2. - 1.;
    return normalize(hlsl::float3(x, y, z));
}

inline hlsl::float3 RandDirectionUniform(GLUE_INOUT(hlsl::uint) state)
{
    float u = PcgRand01(state); // [0,1)
    float v = PcgRand01(state); // [0,1)

    float z = 1.0 - 2.0 * u;
    float r = sqrt(saturate(1.0 - z * z));
    float phi = 2.0 * PI * v;

    return hlsl::float3(r * cos(phi), r * sin(phi), z);
}

inline hlsl::float3 RandHemisphereUniformSSpace(float u1, float u2)
{
    float z = u1;                 // glueCos(theta)
    float r = sqrt(max(0.0f, 1.0f - z*z));
    float phi = 2 * PI * u2;

    return normalize(hlsl::float3(
        r * cos(phi),
        r * sin(phi),
        z
    ));
}

inline hlsl::float3 RandHemisphereCosineSSpace(float u1, float u2)
{
    float r = sqrt(u1);
    float theta = 2.0f * PI * u2;

    float x = r * cos(theta);
    float y = r * sin(theta);
    float z = sqrt(1.0f - u1); // ensures cosine weighting

    return normalize(hlsl::float3(x, y, z));
}

inline hlsl::float3 RandHemisphereCosineWorld(float u1, float u2, ShadingFrame frame)
{
    hlsl::float3 L_s = RandHemisphereCosineSSpace(u1, u2);
    return frame.ToWorld(L_s);
}

inline hlsl::float3 RandHemisphereUniformWorld(float u1, float u2, ShadingFrame frame)
{
    hlsl::float3 L_s = RandHemisphereUniformSSpace(u1, u2);
    return frame.ToWorld(L_s);
}

#endif