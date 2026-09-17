#ifndef H_OUTPUT_COLOR_H
#define H_OUTPUT_COLOR_H

#ifdef __cplusplus
#   include "PathTracing/Flags/MethodsCpp.h"
#else
#   include "PathTracing/Flags/MethodsHlsl.hlsli"
#endif

#define PROCESS_AS_ENUM
#include "PathTracing/Debug/OutputColorProcessing.h"
#undef PROCESS_AS_ENUM

#ifdef __cplusplus
#   define PROCESS_AS_STRINGS
#   include "PathTracing/Debug/OutputColorProcessing.h"
#   undef PROCESS_AS_STRINGS
static constexpr auto s_defaultOutputIndex = DebugOutputIndex::eDebugOutput_Disabled;
#endif

#if defined(__cplusplus) || DEBUG_ENABLED_PP(PathDumper)
struct DebugOutputPair
{
    hlsl::float3 Value;
    hlsl::uint IsAssigned;
};

struct DebugOutputStruct
{
    DebugOutputPair Array[(hlsl::uint)DebugOutputIndex::eCount];
};
#else
struct DebugOutputStruct {};
#endif

#endif