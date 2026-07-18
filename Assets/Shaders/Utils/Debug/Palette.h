#ifndef H_DEBUGPALETTE_H
#define H_DEBUGPALETTE_H

#include "Utils/HlslGlue.h"

inline hlsl::float3 Palette(hlsl::uint idx)
{
    const hlsl::float3 colors[20] = {
        hlsl::float3(1.0, 0.0, 0.0),   // Red
        hlsl::float3(0.0, 1.0, 0.0),   // Green
        hlsl::float3(0.0, 0.0, 1.0),   // Blue
        hlsl::float3(1.0, 1.0, 0.0),   // Yellow
        hlsl::float3(1.0, 0.0, 1.0),   // Magenta
        hlsl::float3(0.0, 1.0, 1.0),   // Cyan
        hlsl::float3(1.0, 0.5, 0.0),   // Orange
        hlsl::float3(0.5, 0.0, 1.0),   // Purple
        hlsl::float3(0.0, 0.5, 1.0),   // Sky blue
        hlsl::float3(0.5, 1.0, 0.0),   // Lime
        hlsl::float3(1.0, 0.0, 0.5),   // Pink-red
        hlsl::float3(0.0, 1.0, 0.5),   // Aqua
        hlsl::float3(0.5, 0.5, 0.5),   // Gray
        hlsl::float3(1.0, 0.75, 0.0),  // Amber
        hlsl::float3(0.75, 0.25, 0.0), // Brownish
        hlsl::float3(0.25, 0.75, 1.0), // Light blue
        hlsl::float3(0.75, 0.0, 0.75), // Violet
        hlsl::float3(0.25, 1.0, 0.25), // Light green
        hlsl::float3(1.0, 0.25, 0.25), // Light red
        hlsl::float3(0.25, 0.25, 1.0)  // Light blue
    };

    return colors[idx % 20];
}

#endif
