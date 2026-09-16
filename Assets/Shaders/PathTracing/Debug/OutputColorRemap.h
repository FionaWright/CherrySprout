#ifndef H_OUTPUT_COLOR_REMAP_H
#define H_OUTPUT_COLOR_REMAP_H

#include "Utils/HlslGlue.h"
#include "Utils/Debug/Palette.h"

enum class Internal_DebugOutputColorRemapIdx : hlsl::uint
{
    eIdx_UnormToSnorm,
    eIdx_SnormToUnorm,
    eIdx_Normalize,
    eIdx_Absolute,
    eIdx_SquareRoot,
    eIdx_MaxComponent,
    eIdx_IsNonZero,
    eIdx_Log,
    eIdx_Magnitude,
    eIdx_Palette,
    eCount,
};

enum class DebugOutputColorRemap : hlsl::uint
{
    eNone = 0,

    eUnormToSnorm = 1u << (hlsl::uint)Internal_DebugOutputColorRemapIdx::eIdx_UnormToSnorm,
    eSnormToUnorm = 1u << (hlsl::uint)Internal_DebugOutputColorRemapIdx::eIdx_SnormToUnorm,
    eNormalize = 1u << (hlsl::uint)Internal_DebugOutputColorRemapIdx::eIdx_Normalize,
    eAbsolute = 1u << (hlsl::uint)Internal_DebugOutputColorRemapIdx::eIdx_Absolute,
    eSquareRoot = 1u << (hlsl::uint)Internal_DebugOutputColorRemapIdx::eIdx_SquareRoot,
    eMaxComponent = 1u << (hlsl::uint)Internal_DebugOutputColorRemapIdx::eIdx_MaxComponent,
    eIsNonZero = 1u << (hlsl::uint)Internal_DebugOutputColorRemapIdx::eIdx_IsNonZero,
    eLog = 1u << (hlsl::uint)Internal_DebugOutputColorRemapIdx::eIdx_Log,
    eMagnitude = 1u << (hlsl::uint)Internal_DebugOutputColorRemapIdx::eIdx_Magnitude,
    ePalette = 1u << (hlsl::uint)Internal_DebugOutputColorRemapIdx::eIdx_Palette,
};

#ifdef __cplusplus

static const char* s_debugOutputColorRemapNames[static_cast<hlsl::uint>(Internal_DebugOutputColorRemapIdx::eCount)] =
{
    "UnormToSnorm",
    "SnormToUnorm",
    "Normalize",
    "Absolute",
    "SquareRoot",
    "MaxComponent",
    "IsNonZero",
    "Log",
    "Magnitude",
    "Palette"
};
static constexpr auto s_defaultOutputColorRemap = DebugOutputColorRemap::eNone;

#else

#   if DEBUG_ENABLED_PP(OutputColor)

[noinline]
float3 ApplyRemap(float3 color, uint outputColorRemap)
{
    if (outputColorRemap & (uint)DebugOutputColorRemap::eUnormToSnorm)
        color = RemapUtoS(color);

    if (outputColorRemap & (uint)DebugOutputColorRemap::eSnormToUnorm)
        color = RemapStoU(color);

    if (outputColorRemap & (uint)DebugOutputColorRemap::eNormalize)
        color = normalize(color);

    if (outputColorRemap & (uint)DebugOutputColorRemap::eAbsolute)
        color = abs(color);

    if (outputColorRemap & (uint)DebugOutputColorRemap::eSquareRoot)
        color = sqrt(color);

    if (outputColorRemap & (uint)DebugOutputColorRemap::eMaxComponent)
        color = max(color.x, max(color.g, color.b));

    if (outputColorRemap & (uint)DebugOutputColorRemap::eIsNonZero)
        color = float3(color.x != 0.0f, color.y != 0.0f, color.z != 0.0f);

    if (outputColorRemap & (uint)DebugOutputColorRemap::eLog)
        color = log(color);

    if (outputColorRemap & (uint)DebugOutputColorRemap::eMagnitude)
        color = length(color);

    if (outputColorRemap & (uint)DebugOutputColorRemap::ePalette)
        color = Palette(color.x);

    return color;
}

#       define DBG_OUTPUT_COLOR_REMAP(color) { color = ApplyRemap(color, gDebugSettings.OutputColorRemapIdx); }

#   else

#       define DBG_OUTPUT_COLOR_REMAP(color)

#   endif

#endif

#endif