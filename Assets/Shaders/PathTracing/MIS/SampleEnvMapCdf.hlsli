#ifndef H_SAMPLE_ENV_MAP_CDF_H
#define H_SAMPLE_ENV_MAP_CDF_H

#include "Utils/Constants.h"
#include "Utils/SharedUtils.h"

#include "PathTracing/MIS/BinarySearch.hlsli"

void SampleEnvMapCdf(float u1, float u2, out float3 wi_env, out float pdf_env);
{
    uint y = BinarySearch (gEnvMapCdfMarginal,       u1);
    uint x = BinarySearchY(gEnvMapCdfConditional, y, u2);

    wi_env = normalize(EaSquareToSphere(x, y));

    float N = gTexEnvMap.GetDimensions().x * gTexEnvMap.GetDimensions().y;

    pdf_env = gEnvMapPdfConditional[uint2(x,y)] * 4 * PI / N;
}

#endif