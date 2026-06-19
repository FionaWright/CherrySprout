#ifndef H_SAMPLE_ENV_MAP_CDF_H
#define H_SAMPLE_ENV_MAP_CDF_H

#include "Utils/Constants.h"
#include "Utils/SharedUtils.h"

#include "PathTracing/MIS/BinarySearch.hlsli"

void SampleEnvMapCdf(float u1, float u2, out float2 uv, out float3 wi, out float pdf)
{
    uint2 dim;
    gTexEnvMap.GetDimensions(dim.x, dim.y);

    DBG_ASSERT_RANGE(0.999f, gEnvMapCdfMarginal[dim.x - 1], 1.001f,        CDF_END_IN_ONE);

    uint y = BinarySearch (gEnvMapCdfMarginal,       u1);
    uint x = BinarySearchX(gEnvMapCdfConditional, y, u2);

    uv = uint2(x, y) / float2(dim);
    wi = normalize(EaSquareToSphere(uv));

    float N = dim.x * dim.y;

    float pixelArea = 4 * PI / N;
    pdf = gEnvMapPmfConditional[uint2(x,y)] / pixelArea;
}

#endif