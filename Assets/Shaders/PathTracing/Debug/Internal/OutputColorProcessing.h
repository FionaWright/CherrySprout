#include "Utils/HlslGlue.h"

#ifdef PROCESS_AS_ENUM

#define PROCESS_OUTPUT_COLOR(label) eDebugOutput_##label,

enum class DebugOutputIndex : hlsl::uint
{

#elif defined(PROCESS_AS_STRINGS)

#define PROCESS_OUTPUT_COLOR(label) #label,

static const char* s_debugOutputIdxNames[static_cast<hlsl::uint>(DebugOutputIndex::eCount)] =
{

#else
#   error
#endif

#define PROCESSING_OUTPUT_COLOR_LIST
#include "PathTracing/Debug/OutputColorList.h"
#undef PROCESSING_OUTPUT_COLOR_LIST

#ifdef PROCESS_AS_ENUM
    eCount
#endif

};

#undef PROCESS_OUTPUT_COLOR