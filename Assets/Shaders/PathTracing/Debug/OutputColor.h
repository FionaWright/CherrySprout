#ifndef H_OUTPUT_COLOR_H
#define H_OUTPUT_COLOR_H

#define PROCESS_AS_ENUM
#include "PathTracing/Debug/OutputColorProcessing.h"
#undef PROCESS_AS_ENUM

#ifdef __cplusplus
#   define PROCESS_AS_STRINGS
#   include "PathTracing/Debug/OutputColorProcessing.h"
#   undef PROCESS_AS_STRINGS

static constexpr auto s_defaultOutputIndex = DebugOutputIndex::eDebugOutput_Disabled;
#endif

#endif