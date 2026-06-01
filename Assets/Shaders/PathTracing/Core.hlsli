#ifndef H_CORE_H
#define H_CORE_H

#include "PathTracing/Buffers.hlsli"
#include "PathTracing/Accumulate.hlsli"

void Core(uint2 pixelCoord)
{
    float3 c = AccumulateAndFetch(pixelCoord, COLOR_RED, true);

    gOutput[pixelCoord].rgb = c;
}

#endif