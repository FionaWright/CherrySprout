#ifndef H_SAMPLE_ENV_MAP_CDF_H
#define H_SAMPLE_ENV_MAP_CDF_H

#include "Utils/Constants.h"
#include "Utils/SharedUtils.h"

#include "PathTracing/MIS/BinarySearch.hlsli"

void SampleEnvMapCdf(float u1, float u2, out float3 wi_env, out float pdf_env)
{
    uint2 dim;
    gTexEnvMap.GetDimensions(dim.x, dim.y);

    DBG_ASSERT_RANGE(0.999f, gEnvMapCdfMarginal[dim.x - 1], 1.001f,        CDF_END_IN_ONE);

    uint y = BinarySearch (gEnvMapCdfMarginal,       u1);
    uint x = BinarySearchX(gEnvMapCdfConditional, y, u2);

    //float2 uv = float2(u1, u2);
    float2 uv = uint2(x, y) / float2(dim);
    wi_env = normalize(EaSquareToSphere(uv));

    //DBG_FORCE_OUTPUT3(gTexEnvMap.Sample(gSampler, uv).rgb);

    float N = dim.x * dim.y;

    // TODO: Write a comment explaining the 4 pi / N thing in detail
    pdf_env = gEnvMapPmfConditional[uint2(x,y)] * 4 * PI / N;
}

#endif