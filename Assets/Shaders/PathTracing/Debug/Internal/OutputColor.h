#ifndef H_OUTPUT_COLOR_H
#define H_OUTPUT_COLOR_H

#ifdef __cplusplus
#   include "../../FeatFlags/FeatFlags.hpp"
#else
#   include "PathTracing/FeatFlags/FeatFlags.hlsli"
#endif

#define PROCESS_AS_ENUM
#include "OutputColorProcessing.h"
#undef PROCESS_AS_ENUM

#ifdef __cplusplus
#   define PROCESS_AS_STRINGS
#include "OutputColorProcessing.h"
#   undef PROCESS_AS_STRINGS
static constexpr auto s_defaultOutputIndex = DebugOutputIndex::eDebugOutput_Disabled;
#endif

#if defined(__cplusplus) || FEAT_DBG_D(PathDumper)
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