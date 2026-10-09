#ifndef H_DEBUG_STRUCTS_H
#define H_DEBUG_STRUCTS_H

#include "PathTracing/Structs.h"
#include "Utils/HlslGlue.h"
#include "PathTracing/GradientDomain/Structs.h"

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
    hlsl::uint Explored;
    PathState PathState;
    PathVertexInfo PathVertex;
    hlsl::float3 ForcedOutput;
};

struct SumSquaredErrorStruct
{
    float SquaredError;
};

#endif