#ifndef H_PATH_DUMP_H
#define H_PATH_DUMP_H

#if DEBUG_ENABLED_PP(PathDumper)

#include "PathTracing/Debug/Globals.hlsli"

void ClearBuffer()
{
    if (any(gDebugPixelCoord != gDebugSettings.ChosenPixelCoords))
        return;

    for (int i = 0; i < gSettings.MaxRayDepth; i++)
    {
        gPathDump[i].Explored = false;
        gPathDump[i].PathState.LastBxdfPdf = NAN;
        gPathDump[i].PathState.RaySegmentIdx = 0;
        gPathDump[i].PathState.LastRayWasDiracDelta = false;

        for (int j = 0; j < (int)DebugOutputIndex::eCount; j++)
        {
            gPathDump[i].DebugOutputs.Float3List[j] = NAN;
            gPathDump[i].DebugOutputs.IsAssignedValueList[j] = false;
        }
    }
}

void AssignDebugOutput(float3 value, uint dbgIdx)
{
    if (any(gDebugPixelCoord != gDebugSettings.ChosenPixelCoords))
        return;

    gPathDump[gDebugCurrentRayDepth].DebugOutputs.Float3List[dbgIdx] = value;
    gPathDump[gDebugCurrentRayDepth].DebugOutputs.IsAssignedValueList[dbgIdx] = true;
}

void AssignPathState(PathState pathState)
{
    if (any(gDebugPixelCoord != gDebugSettings.ChosenPixelCoords))
        return;

    gPathDump[gDebugCurrentRayDepth].PathState = pathState;
}

void MarkExplored()
{
    if (any(gDebugPixelCoord != gDebugSettings.ChosenPixelCoords))
        return;

    gPathDump[gDebugCurrentRayDepth].Explored = true;
}

#define HIGHLIGHT_CIRCLE_MIN 3
#define HIGHLIGHT_CIRCLE_MAX 8

void Highlight(inout float3 color)
{
    float dist = length((float2)gDebugSettings.ChosenPixelCoords - (float2)gDebugPixelCoord);
    if (HIGHLIGHT_CIRCLE_MIN < dist && dist < HIGHLIGHT_CIRCLE_MAX)
    {
        float2 uv = (float2)gDebugPixelCoord / (float2)gSettings.FrameDimensions;
        uv = uv * 2.0 - 1.0;

        float t = gSettings.FrameIdx * 0.005;

        float r = length(uv);
        float a = atan2(uv.y, uv.x);

        float phase =
              8.0 * r
            + 5.0 * a
            + 2.0 * sin(6.0 * r - 3.0 * t)
            + t;

        color = 0.5 + 0.5 * cos(
            phase +
            float3(0.0,
                   2.0943951,   // 2π/3
                   4.1887902)); // 4π/3

        color.r = max(0.6f, color.r);
        color *= float3(1.2, 0.8, 0);
    }
}

#   define DBG_PATH_DUMP_CLEAR() { ClearBuffer(); }
#   define DBG_PATH_DUMP_DEBUG_OUTPUT(value, dbgIdx) { AssignDebugOutput(value, dbgIdx); }
#   define DBG_PATH_DUMP_PATH_STATE(pathState) { AssignPathState(pathState); }
#   define DBG_PATH_DUMP_MARK_EXPLORED() { MarkExplored(); }
#   define DBG_PATH_DUMP_HIGHLIGHT(color) { Highlight(color); }

#else

#   define DBG_PATH_DUMP_CLEAR()
#   define DBG_PATH_DUMP_DEBUG_OUTPUT(dbgIdx, value)
#   define DBG_PATH_DUMP_PATH_STATE(pathState)
#   define DBG_PATH_DUMP_MARK_EXPLORED()
#   define DBG_PATH_DUMP_HIGHLIGHT(color)

#endif

#endif