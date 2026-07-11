#ifndef H_DEBUG_STRUCTS_H
#define H_DEBUG_STRUCTS_H

#include "PathTracing/Structs.h"
#include "Utils/HlslGlue.h"
#include "PathTracing/Debug/OutputColor.h"

struct DebugErrorInfo
{
    hlsl::uint ExprCounter;
    hlsl::uint NaNCounter;
    hlsl::uint InfCounter;

    hlsl::float4 Value1;
    hlsl::float4 Value2;
    hlsl::float4 Value3;

    hlsl::uint2 PixelCoord;
    hlsl::uint FrameIndex;
};

struct RayDump
{
    DebugOutputStruct DebugOutputs;
    PathState PathState;
};

#endif