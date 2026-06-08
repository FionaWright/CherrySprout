#ifndef H_NAN_UTILS_H
#define H_NAN_UTILS_H

// https://sakibsaikia.github.io/graphics/2022/01/04/Nan-Checks-In-HLSL.html

bool IsNaN(float x)
{
    return (asuint(x) & 0x7fffffff) > 0x7f800000;
}

bool IsNaN3(float3 x)
{
    return IsNaN(x.x) || IsNaN(x.y) || IsNaN(x.z);
}

bool IsInf(float x)
{
    return !IsNaN(x) && IsNaN(x * 0.0);
}

bool IsInf3(float3 x)
{
    return IsInf(x.x) || IsInf(x.y) || IsInf(x.z);
}

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