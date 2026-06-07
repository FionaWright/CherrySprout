#ifndef H_DBG_OUTPUT_COLOR_H
#define H_DBG_OUTPUT_COLOR_H

#include "Utils/HlslGlue.h"
#include "PathTracing/Flags.h"

enum class DebugOutputIndex : hlsl::uint
{
    eDebugOutput_NormalShaded,
    eDebugOutput_NormalShadedFF,
    eDebugOutput_NormalGeometricFF,
    eDebugOutput_UV,

    eDebugOutput_Albedo,
    eDebugOutput_Emission,
    eDebugOutput_Barycentrics,

    eCount
};

#ifdef __cplusplus

static const char* s_debugOutputIdxNames[(int)DebugOutputIndex::eCount] = {
    "Normals Shaded",
    "Normals Shaded FF",
    "Normals Geo FF",
    "UV",
    "Albedo",
    "Emission",
    "Barycentrics",
};

static constexpr DebugOutputIndex s_defaultOutputIndex = DebugOutputIndex::eDebugOutput_NormalShaded;

#else

#    ifndef DEBUG_OUTPUT_COLOR
#        define DEBUG_OUTPUT_COLOR DebugOutputIndex::eDebugOutput_NormalShaded
#    endif

#    define DBG_OUTPUT_START() dbgOutput = NAN;

#    define DBG_OUTPUT3(value, idx) if (DEBUG_ENABLED(OutputColor) && DebugOutputIndex::eDebugOutput_##idx == (DebugOutputIndex)(DEBUG_OUTPUT_COLOR)) { dbgOutput = value; return; }
#    define DBG_OUTPUT2(value, idx) if (DEBUG_ENABLED(OutputColor) && DebugOutputIndex::eDebugOutput_##idx == (DebugOutputIndex)(DEBUG_OUTPUT_COLOR)) { dbgOutput = float3(value, 0); return; }
#    define DBG_OUTPUT1(value, idx) if (DEBUG_ENABLED(OutputColor) && DebugOutputIndex::eDebugOutput_##idx == (DebugOutputIndex)(DEBUG_OUTPUT_COLOR)) { dbgOutput = value.xxx; return; }

#endif

#endif