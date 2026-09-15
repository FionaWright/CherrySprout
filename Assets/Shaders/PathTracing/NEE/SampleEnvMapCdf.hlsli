#ifndef H_SAMPLE_ENV_MAP_CDF_H
#define H_SAMPLE_ENV_MAP_CDF_H

#if FEATURE_ENABLED(NEE)

#include "Utils/Constants.h"
#include "Utils/SharedUtils.h"

#include "PathTracing/NEE/BinarySearch.hlsli"

void SampleEnvMapCdf(float xi1, float xi2, out float2 uv, out float3 wi, out float pdf)
{
    uint2 dim;
    gTexEnvMap.GetDimensions(dim.x, dim.y);
    float N = dim.x * dim.y;

    DBG_ASSERT_RANGE(0.999f, gEnvMapCdfMarginal[dim.x - 1], 1.001f,        CDF_END_IN_ONE);

    uint y = BinarySearch (gEnvMapCdfMarginal,       xi1);
    uint x = BinarySearchX(gEnvMapCdfConditional, y, xi2);

    uv = uint2(x, y) / float2(dim);
    wi = normalize(EaSquareToSphere(uv));

    float pixelArea = 4 * PI / N;
    pdf = gEnvMapPmfConditional[uint2(x,y)] / pixelArea;
}

float GetEnvMapPdf(float2 uv)
{
    uint2 dim;
    gTexEnvMap.GetDimensions(dim.x, dim.y);
    float N = dim.x * dim.y;

    uint2 xy = uv * dim;

    float pixelArea = 4 * PI / N;
    return gEnvMapPmfConditional[xy] / pixelArea;
}

#else

void SampleEnvMapCdf(float xi1, float xi2, out float2 uv, out float3 wi, out float pdf)
{
    uv = NAN;
    wi = NAN;
    pdf = NAN;
}

float GetEnvMapPdf(float2 uv) { return NAN; }

#endif

#endif