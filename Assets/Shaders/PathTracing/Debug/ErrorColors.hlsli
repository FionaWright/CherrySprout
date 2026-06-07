#ifndef H_ERROR_COLORS_H
#define H_ERROR_COLORS_H

#include "Utils/HlslUtils.hlsli"

float3 GetNaNVisualizerColor(uint2 pixelCoord)
{
    uint x = gSettings.FrameIdx / 10 + pixelCoord.x + pixelCoord.y * gSettings.FrameDimensions.x;
    return float3(1, (x / 4) % 8 == 0, 1);
}

float3 GetInfVisualizerColor(uint2 pixelCoord)
{
    uint x = pixelCoord.x + pixelCoord.y * gSettings.FrameDimensions.x - gSettings.FrameIdx / 10;
    return float3(0, (x / 4) % 8 == 0, 1);
}

#endif