#ifndef H_PT_DEBUG_GLOBALS_H
#define H_PT_DEBUG_GLOBALS_H

#if FEAT_DBG_D(OutputColor) || FEAT_DBG_D(PathDumper) || FEAT_DBG_D(Asserts)

#include "Utils/Constants.h"

static bool gDebugValueFound = false;
static float3 gDebugValue = NAN;
static int gDebugCurrentRayDepth = 0;
static uint2 gDebugPixelCoord = uint2(UINT_MAX, UINT_MAX);
static uint gDebugFrameIndex = UINT_MAX;
static uint gDebugNumAssertsTriggered = 0;

#    define DBG_SET_PIXEL_INFO(pixelCoord, frameIndex) { gDebugPixelCoord = pixelCoord; gDebugFrameIndex = frameIndex; }
#    define DBG_SET_CURRENT_RAY_DEPTH(depth) { gDebugCurrentRayDepth = depth; }

#else

#    define DBG_SET_PIXEL_INFO(pixelCoord, frameIndex)
#    define DBG_SET_CURRENT_RAY_DEPTH(depth)

#endif

#endif