#ifndef H_OUTPUT_COLOR_MACROS_H
#define H_OUTPUT_COLOR_MACROS_H

#include "PathTracing/Flags/MethodsHlsl.hlsli"
#include "PathTracing/Debug/Globals.hlsli"
#include "PathTracing/Debug/PathDumper.hlsli"

#if DEBUG_ENABLED_PP(OutputColor)

#include "PathTracing/Debug/OutputColor.h"
#include "PathTracing/Debug/OutputColorRemap.h"

[noinline]
void dbgOutput3(float value_x, float value_y, float value_z, uint index)
{
    float3 value = float3(value_x, value_y, value_z);

    DBG_PATH_DUMP_DEBUG_OUTPUT(value, index);

    if (!DEBUG_ENABLED(OutputColor) || gDebugSettings.OutputColorIdx == (uint)DebugOutputIndex::eDebugOutput_Disabled)
        return;

    if (index != gDebugSettings.OutputColorIdx)
        return;

    if (gDebugValueFound)
        return;

    if (gDebugSettings.ChosenRayDepth != -1 && gDebugSettings.ChosenRayDepth != gDebugCurrentRayDepth)
        return;

    DBG_OUTPUT_COLOR_REMAP(value);

    gDebugValueFound = true;
    gDebugValue = value;
}

void dbgOutputSet(inout float3 output)
{
    if (!DEBUG_ENABLED(OutputColor) || gDebugSettings.OutputColorIdx == 0)
        return;

    output = gDebugValue;
}

#    define DBG_OUTPUT3(value, label) dbgOutput3(value.x, value.y, value.z, (uint)DebugOutputIndex::eDebugOutput_##label);
#    define DBG_OUTPUT2(value, label) DBG_OUTPUT3(float3(value.xy, 0), label)
#    define DBG_OUTPUT1(value, label) DBG_OUTPUT3(value.xxx, label)
#    define DBG_FORCE_OUTPUT3(value) { gDebugValueFound = true; gDebugValue = value; }

#    define DBG_OUTPUT_SET(output) dbgOutputSet(output);
#    define DBG_OUTPUT_RESET() { gDebugValueFound = false; gDebugValue = NAN; }

#elif DEBUG_ENABLED_PP(PathDumper)

#    define DBG_OUTPUT3(value, label) DBG_PATH_DUMP_DEBUG_OUTPUT(value, (uint)DebugOutputIndex::eDebugOutput_##label)
#    define DBG_OUTPUT2(value, label) DBG_OUTPUT3(float3(value, 0), label)
#    define DBG_OUTPUT1(value, label) DBG_OUTPUT3(value.xxx, label)

#    define DBG_FORCE_OUTPUT3(value)
#    define DBG_OUTPUT_SET(output)
#    define DBG_OUTPUT_RESET()

#else

#    define DBG_OUTPUT3(value, label)
#    define DBG_OUTPUT2(value, label)
#    define DBG_OUTPUT1(value, label)
#    define DBG_FORCE_OUTPUT3(value)
#    define DBG_OUTPUT_SET(output)
#    define DBG_OUTPUT_RESET()

#endif

#endif