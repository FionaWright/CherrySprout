#ifndef H_PT_DEBUG_GLOBALS_H
#define H_PT_DEBUG_GLOBALS_H

#if DEBUG_ENABLED(OutputColor) || DEBUG_ENABLED(PathDumper) || DEBUG_ENABLED(Asserts)

static bool gDebugValueFound = false;
static float3 gDebugValue = NAN;
static int gDebugCurrentRayDepth = 0;
static uint2 gDebugPixelCoord = uint2(UINT_MAX, UINT_MAX);
static uint gDebugFrameIndex = UINT_MAX;

#    define DBG_ASSERT_SET_PIXEL_INFO(pixelCoord, frameIndex) { gDebugPixelCoord = pixelCoord; gDebugFrameIndex = frameIndex; }
#    define DBG_SET_CURRENT_RAY_DEPTH(depth) { gDebugCurrentRayDepth = depth; }

#else

#    define DBG_ASSERT_SET_PIXEL_COORD(pixelCoord)
#    define DBG_SET_CURRENT_RAY_DEPTH(depth)

#endif

#endif