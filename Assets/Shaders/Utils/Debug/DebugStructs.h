#ifndef H_DEBUG_STRUCTS_H
#define H_DEBUG_STRUCTS_H
#include "Utils/HlslGlue.h"

struct DebugErrorInfo
{
    hlsl::uint ExprCounter;
    hlsl::uint NaNCounter;
    hlsl::uint InfCounter;

    hlsl::float4 Value1;
    hlsl::float4 Value2;
    hlsl::float4 Value3;
};

#endif