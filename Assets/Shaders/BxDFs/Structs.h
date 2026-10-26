#ifndef H_BXDF_STRUCTS_H
#define H_BXDF_STRUCTS_H

#include "Utils/HlslGlue.h"
#include "Utils/Constants.h"

enum class LobeTag : hlsl::uint
{
    eNone = 0,

    eLambertian = 1 << 0,
    eSpecular = 1 << 2,
    eTranmissiveReflect = 1 << 3,
    eTransmissiveRefract = 1 << 4,
    eClearcoat = 1 << 5,
    eSheen = 1 << 6,

    eNEE = 1 << 7,

    eAll = UINT_MAX
};

inline bool CoversLobe(const LobeTag state, const LobeTag tag)
{
    return (hlsl::uint)state & (hlsl::uint)tag;
}

inline bool MaterialIsDirac(const float roughness)
{
    return roughness < ROUGHNESS_DIRAC_THRESHOLD;
}

inline bool IsDirac(const LobeTag lobe, const float roughness)
{
    return MaterialIsDirac(roughness) && (
            CoversLobe(lobe, LobeTag::eSpecular) ||
            CoversLobe(lobe, LobeTag::eTranmissiveReflect) ||
            CoversLobe(lobe, LobeTag::eTransmissiveRefract) ||
            CoversLobe(lobe, LobeTag::eClearcoat)
        );
}

struct BxdfSample
{
    hlsl::float3 Wi;
    bool IsDirac;
    LobeTag Lobe;
};

struct BxdfEval
{
    hlsl::float3 Radiance;
    float PDF;
};

#endif