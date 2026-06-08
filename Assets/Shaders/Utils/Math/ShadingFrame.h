#ifndef H_SHADING_FRAME_H
#define H_SHADING_FRAME_H

#include "Utils/SharedUtils.h"

struct ShadingFrame
{
    void Init(hlsl::float3 n);

    hlsl::float3 ToLocal(hlsl::float3 W);
    hlsl::float3 ToWorld(hlsl::float3 W_s);

    hlsl::float3 T;
    hlsl::float3 B;
    hlsl::float3 N;
};

inline ShadingFrame CreateShadingFrame(const hlsl::float3 n)
{
    ShadingFrame frame;
    frame.Init(n);
    return frame;
}

inline void ShadingFrame::Init(const hlsl::float3 n)
{
    N = n;
    BuildBasisFrisvad(N, T, B);
}

inline hlsl::float3 ShadingFrame::ToLocal(const hlsl::float3 W)
{
    return normalize(
            hlsl::float3(
                dot(W, T), dot(W, B), dot(W, N)
            )
        );
}

inline hlsl::float3 ShadingFrame::ToWorld(const hlsl::float3 W_s)
{
    return normalize(W_s.x * T + W_s.y * B + W_s.z * N);
}

#endif