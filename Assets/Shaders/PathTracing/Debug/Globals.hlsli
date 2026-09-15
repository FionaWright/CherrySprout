#ifndef H_PT_DEBUG_GLOBALS_H
#define H_PT_DEBUG_GLOBALS_H

#if DEBUG_ENABLED_PP(OutputColor) || DEBUG_ENABLED_PP(PathDumper) || DEBUG_ENABLED_PP(Asserts)

static bool gDebugValueFound = false;
static float3 gDebugValue = NAN;
static int gDebugCurrentRayDepth = 0;
static uint2 gDebugPixelCoord = uint2(UINT_MAX, UINT_MAX);
static uint gDebugFrameIndex = UINT_MAX;

#    define DBG_SET_PIXEL_INFO(pixelCoord, frameIndex) { gDebugPixelCoord = pixelCoord; gDebugFrameIndex = frameIndex; }
#    define DBG_SET_CURRENT_RAY_DEPTH(depth) { gDebugCurrentRayDepth = depth; }

#else

#    define DBG_SET_PIXEL_INFO(pixelCoord, frameIndex)
#    define DBG_SET_CURRENT_RAY_DEPTH(depth)

#endif

#endif