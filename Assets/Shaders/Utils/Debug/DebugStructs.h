#ifndef H_DEBUG_STRUCTS_H
#define H_DEBUG_STRUCTS_H

#include "PathTracing/Structs.h"
#include "Utils/HlslGlue.h"
#include "PathTracing/Debug/OutputColor.h"
#include "PathTracing/GradientDomain/Structs.h"

enum DebugErrorInfoLock : hlsl::uint
{
    eUnlocked,
    eLocked,
};

struct DebugErrorInfo
{
    hlsl::uint Lock;

    hlsl::uint ExprCounter;
    hlsl::uint NaNCounter;
    hlsl::uint InfCounter;

    hlsl::float4 Value1;
    hlsl::float4 Value2;
    hlsl::float4 Value3;

    // For re-running the error:
    hlsl::uint2 PixelCoord;
    hlsl::uint FrameIndex;
    hlsl::float3 CameraPositionWorld;
    hlsl::float4x4 InvV;
};

// sizeof(DebugOutputStruct) = (143 x 3 x 4) + (143 x 4) = 2288
// sizeof(PathState) = 4 x (4 + 4 + 4 + 4 + 4) = 80
// sizeof(PathVertexInfo) = 4 x (1 + 3 + 3x3 + 3 + 3 + 1 + 3 + 1) = 96
// Total = 1 + 2288 + 80 + 96 = 2465

// Solution:
// - Put debug output struct on their own byte address buffer

struct RayDump
{
    hlsl::uint Explored;
    PathState PathState;
    PathVertexInfo PathVertex;
};

#endif