#ifndef H_DEBUG_STRUCTS_H
#define H_DEBUG_STRUCTS_H

#include "PathTracing/Structs.h"
#include "Utils/HlslGlue.h"
#include "PathTracing/Debug/OutputColor.h"

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

struct RayDump
{
    bool Explored;
    DebugOutputStruct DebugOutputs;
    PathState PathState;
};

#endif