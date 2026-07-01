#ifndef H_HIT_INFO_H
#define H_HIT_INFO_H

#include "Scene/Material.h"

#include "Utils/HlslGlue.h"
#include "Utils/Math/ShadingFrame.h"

struct HitInfo
{
    Material Mat;

    hlsl::float3 Ng_ff;
    hlsl::float3 Ns_ff;
    ShadingFrame SFrame;

    hlsl::float3 Li;

    hlsl::float2 AnisoDir;
    float AnisoStrength;

    hlsl::float2 UV;
    bool IsEntering;
    float RayT;
};

#endif