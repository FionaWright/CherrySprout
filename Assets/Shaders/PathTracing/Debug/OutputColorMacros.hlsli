#ifndef H_OUTPUT_COLOR_MACROS_H
#define H_OUTPUT_COLOR_MACROS_H

#include "PathTracing/Debug/OutputColor.h"
#include "PathTracing/Flags/MethodsHlsl.hlsli"
#include "PathTracing/Debug/Globals.hlsli"
#include "PathTracing/Debug/PathDumper.hlsli"
#include "PathTracing/Debug/OutputColorRemap.h"

#if DEBUG_ENABLED(OutputColor)

#    ifndef DEBUG_OUTPUT_COLOR
#        define DEBUG_OUTPUT_COLOR DebugOutputIndex::eDebugOutput_Disabled
#    endif

#    ifndef DEBUG_CHOSEN_RAY_DEPTH
#        define DEBUG_CHOSEN_RAY_DEPTH -1
#    endif

void dbgOutput3(float3 value)
{
    if (!DEBUG_ENABLED(OutputColor))
        return;

    if (gDebugValueFound)
        return;

    if (DEBUG_CHOSEN_RAY_DEPTH != -1 && DEBUG_CHOSEN_RAY_DEPTH != gDebugCurrentRayDepth)
        return;

    DBG_OUTPUT_COLOR_REMAP(value);

    gDebugValueFound = true;
    gDebugValue = value;
}

#    define DBG_OUTPUT3(value, idx)                                                                                                          \
{                                                                                                                                            \
    DBG_PATH_DUMP_DEBUG_OUTPUT(value, (uint)DebugOutputIndex::eDebugOutput_##idx);                                                           \
    if (DebugOutputIndex::eDebugOutput_##idx == (DebugOutputIndex)(DEBUG_OUTPUT_COLOR))                                                      \
        dbgOutput3(value);                                                                                                                   \
}                                                                                                                                            \

#    define DBG_OUTPUT2(value, idx) DBG_OUTPUT3(float3(value, 0), idx)
#    define DBG_OUTPUT1(value, idx) DBG_OUTPUT3(value.xxx, idx)
#    define DBG_FORCE_OUTPUT3(value) { gDebugValueFound = true; gDebugValue = value; return; }

#    define DBG_OUTPUT_SET(output) { output = gDebugValue; }
#    define DBG_OUTPUT_RESET() { gDebugValueFound = false; gDebugValue = NAN; }

#elif DEBUG_ENABLED(PathDumper)

#    define DBG_OUTPUT3(value, idx) DBG_PATH_DUMP_DEBUG_OUTPUT(value, (uint)DebugOutputIndex::eDebugOutput_##idx)
#    define DBG_OUTPUT2(value, idx) DBG_OUTPUT3(float3(value, 0), idx)
#    define DBG_OUTPUT1(value, idx) DBG_OUTPUT3(value.xxx, idx)

#    define DBG_FORCE_OUTPUT3(value)
#    define DBG_OUTPUT_SET(output)
#    define DBG_OUTPUT_RESET()

#else

#    define DBG_OUTPUT3(value, idx)
#    define DBG_OUTPUT2(value, idx)
#    define DBG_OUTPUT1(value, idx)
#    define DBG_FORCE_OUTPUT3(value)
#    define DBG_OUTPUT_SET(output)
#    define DBG_OUTPUT_RESET()

#endif

#endif