#ifndef H_OUTPUT_COLOR_MACROS_H
#define H_OUTPUT_COLOR_MACROS_H

#include "PathTracing/Flags/MethodsHlsl.hlsli"
#include "PathTracing/Debug/Globals.hlsli"
#include "PathTracing/Debug/PathDumper.hlsli"

#if DEBUG_ENABLED(OutputColor)

#include "PathTracing/Debug/OutputColor.h"
#include "PathTracing/Debug/OutputColorRemap.h"

void dbgOutput3(float3 value)
{
    if (gDebugValueFound)
        return;

    if (gDebugSettings.ChosenRayDepth != -1 && gDebugSettings.ChosenRayDepth != gDebugCurrentRayDepth)
        return;

    DBG_OUTPUT_COLOR_REMAP(value);

    gDebugValueFound = true;
    gDebugValue = value;
}

#    define DBG_OUTPUT3(value, label)                                                                                                          \
{                                                                                                                                            \
    DBG_PATH_DUMP_DEBUG_OUTPUT(value, (uint)DebugOutputIndex::eDebugOutput_##label);                                                           \
    if (DebugOutputIndex::eDebugOutput_##label == (DebugOutputIndex)(gDebugSettings.OutputColorIdx))                                           \
        dbgOutput3(value);                                                                                                                   \
}                                                                                                                                            \

#    define DBG_OUTPUT2(value, label) DBG_OUTPUT3(float3(value, 0), label)
#    define DBG_OUTPUT1(value, label) DBG_OUTPUT3(value.xxx, label)
#    define DBG_FORCE_OUTPUT3(value) { gDebugValueFound = true; gDebugValue = value; return; }

#    define DBG_OUTPUT_SET(output) { output = gDebugValue; }
#    define DBG_OUTPUT_RESET() { gDebugValueFound = false; gDebugValue = NAN; }

#elif DEBUG_ENABLED(PathDumper)

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