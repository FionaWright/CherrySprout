#ifndef H_RAND01_H
#define H_RAND01_H

#include "Utils/HlslGlue.h"
#include "Utils/Constants.h"

struct RngInfo
{
    hlsl::uint SampleIdx;
    //hlsl::uint GlobalSampleIdx;
    //hlsl::uint HashScramble;
    //hlsl::uint BounceBaseDimension;
    hlsl::uint IndependentRngState; // Modified during independent sampling
};

inline hlsl::uint wang_hash(hlsl::uint a) {
    a = (a ^ 61u) ^ (a >> 16);
    a *= 9u;
    a = a ^ (a >> 4);
    a *= 0x27d4eb2du;
    a = a ^ (a >> 15);
    return a;
}

inline hlsl::uint PrngSeed(hlsl::uint2 pixel, hlsl::uint sample_i, hlsl::uint temporal_i, hlsl::uint seed) {
    // Random big primes
    // XOR the temporal frame number to avoid accumulated patterns
    return wang_hash(
        pixel.x * 374761393u +
        pixel.y * 668265263u +
        (sample_i * 1597334677u) ^
        (temporal_i * 3812015801u) ^
        (seed * 2319296101u)
    );
}

inline RngInfo InitializeRngInfo(hlsl::uint2 pixel, hlsl::uint sample_i, hlsl::uint temporal_i, hlsl::uint seed)
{
    RngInfo rngInfo;
    rngInfo.SampleIdx = sample_i;
    rngInfo.IndependentRngState = PrngSeed(pixel, sample_i, temporal_i, seed);
    return rngInfo;
}

// https://www.pcg-random.org/
inline float PcgRand01(GLUE_INOUT(hlsl::uint) state)
{
    state += 0x6D2B79F5u;
    hlsl::uint z = (state ^ (state >> 15)) * (1u | state);
    z ^= z + (z ^ (z >> 7)) * (61u | z);
    return float((z ^ (z >> 14))) / (float)UINT_MAX;
}

inline float Rand01(GLUE_INOUT(RngInfo) rngInfo)
{
    return PcgRand01(rngInfo.IndependentRngState);
}

inline float Rand01_Const(const RngInfo rngInfo)
{
    hlsl::uint discardState = rngInfo.IndependentRngState;
    return PcgRand01(discardState);
}

#endif