#ifndef H_SHADING_FRAME_H
#define H_SHADING_FRAME_H

#include "Utils/MathUtils.h"

struct ShadingFrame
{
    void Init(float3 n);

    hlsl::float3 ToLocal(hlsl::float3 W);
    hlsl::float3 ToWorld(hlsl::float3 W_s);

    hlsl::float3 T;
    hlsl::float3 B;
    hlsl::float3 N;
};

ShadingFrame CreateShadingFrame(hlsl::float3 n)
{
    ShadingFrame frame;
    frame.Init(n);
    return frame;
}

void ShadingFrame::Init(hlsl::float3 n)
{
    N = n;
    BuildBasisFrisvad(N, T, B);
}

hlsl::float3 ShadingFrame::ToLocal(hlsl::float3 W)
{
    return normalize(
            hlsl::float3(
                dot(W, T), dot(W, B), dot(W, N)
            )
        );
}

hlsl::float3 ShadingFrame::ToWorld(hlsl::float3 W_s)
{
    return normalize(W_s.x * T + W_s.y * B + W_s.z * N);
}

#endif