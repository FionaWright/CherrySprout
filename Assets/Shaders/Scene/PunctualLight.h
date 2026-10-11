#ifndef H_PUNCTUAL_LIGHT_H
#define H_PUNCTUAL_LIGHT_H

#include "Utils/HlslGlue.h"

enum class PunctualLightType : hlsl::uint
{
    ePoint,
    eDistant,
    eSpot,
    eRect,
    eCount,
};

struct PunctualLight
{
    PunctualLightType Type;

    hlsl::float3 Position;
    hlsl::float3 Color;
    float Intensity; // TODO: Units?

    // Point
    float PointRadius;

    // Spot
    float SpotInnerAngle;
    float SpotOuterAngle;

    // Rect
    float RectWidth;
    float RectHeight;

    hlsl::float3 SpotOrRectDirection;

#ifdef __cplusplus
    PunctualLight()
    {
        Type = PunctualLightType::ePoint;
        Position = { 0, 0, 0 };
        Color = { 0, 0, 0 };
        Intensity = 0.0f;
        PointRadius = 0.0f;
        SpotInnerAngle = 0.0f;
        SpotOuterAngle = 0.0f;
        SpotOrRectDirection = { 0, 0, 0 };
        RectWidth = 0.0f;
        RectHeight = 0.0f;
    }
#endif
};

#endif