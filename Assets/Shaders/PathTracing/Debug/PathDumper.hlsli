#ifndef H_PATH_DUMP_H
#define H_PATH_DUMP_H

#if DEBUG_ENABLED(PathDumper)

#include "PathTracing/Debug/Globals.hlsli"

#    ifndef DEBUG_CHOSEN_PIXEL_COORDS
#        define DEBUG_CHOSEN_PIXEL_COORDS uint2(UINT_MAX, UINT_MAX)
#    endif

void ClearBuffer()
{
    if (any(gDebugPixelCoord != DEBUG_CHOSEN_PIXEL_COORDS))
        return;

    for (int i = 0; i < gSettings.MaxRayDepth; i++)
    {
        gPathDump[i].PathState.LastBxdfPdf = NAN;
        gPathDump[i].PathState.RaySegmentIdx = 0;
        gPathDump[i].PathState.LastRayDiracDelta = false;

        for (int j = 0; j < (int)DebugOutputIndex::eCount; j++)
            gPathDump[i].DebugOutputs.Float3List[j] = NAN;
    }
}

void AssignDebugOutput(float3 value, uint dbgIdx)
{
    if (any(gDebugPixelCoord != DEBUG_CHOSEN_PIXEL_COORDS))
        return;

    gPathDump[gDebugCurrentRayDepth].DebugOutputs.Float3List[dbgIdx] = value;
}

void AssignPathState(PathState pathState)
{
    gPathDump[gDebugCurrentRayDepth].PathState = pathState;
}

#   define DBG_PATH_DUMP_CLEAR() { ClearBuffer(); }
#   define DBG_PATH_DUMP_DEBUG_OUTPUT(value, dbgIdx) { AssignDebugOutput(value, dbgIdx); }
#   define DBG_PATH_DUMP_PATH_STATE(pathState) { AssignPathState(pathState); }

#else

#   define DBG_PATH_DUMP_CLEAR()
#   define DBG_PATH_DUMP_DEBUG_OUTPUT(dbgIdx, value)
#   define DBG_PATH_DUMP_PATH_STATE(pathState)

#endif

#endif