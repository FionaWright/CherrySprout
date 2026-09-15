#include "Utils/HlslGlue.h"

#define FEATURE_FLAG_TYPE hlsl::uint
#define DEBUG_FLAG_TYPE hlsl::uint

// ====================== Guards ======================

#if defined(FLAGS_PROCESS_AS_INDEX)

#elif defined(FLAGS_PROCESS_AS_FLAG)
#   ifdef FLAGS_PROCESSING_DEBUG
#       ifndef FLAGS_PROCESS_AS_INDEX_COMPLETED_DEBUG
#           error You must process this file for indices before processing for flags (Debug)
#       endif
#   else
#       ifndef FLAGS_PROCESS_AS_INDEX_COMPLETED
#           error You must process this file for indices before processing for flags (Feature)
#       endif
#   endif

#elif defined(FLAGS_PROCESS_AS_STRING)
#   ifdef FLAGS_PROCESSING_DEBUG
#       ifndef FLAGS_PROCESS_AS_FLAG_COMPLETED_DEBUG
#           error You must process this file for flags before processing for strings (Debug)
#       endif
#   else
#       ifndef FLAGS_PROCESS_AS_FLAG_COMPLETED
#           error You must process this file for flags before processing for strings (Feature)
#       endif
#   endif

#elif defined(FLAGS_PROCESS_AS_CBCV)
#   ifdef FLAGS_PROCESSING_DEBUG
#       ifndef FLAGS_PROCESS_AS_FLAG_COMPLETED_DEBUG
#           error You must process this file for flags before processing for CBCV (Debug)
#       endif
#   else
#       ifndef FLAGS_PROCESS_AS_FLAG_COMPLETED
#           error You must process this file for flags before processing for CBCV (Feature)
#       endif
#   endif

#else
#   error Invalid processing of file

#endif

// ================ FLAG_PROCESS Macro ================

#ifdef FLAG_PROCESS
#   undef FLAG_PROCESS
#endif

#if defined(FLAGS_PROCESS_AS_INDEX)
#   define FLAG_PROCESS(flag, CanBeCbvValue) Idx_##flag,

#elif defined(FLAGS_PROCESS_AS_FLAG)
#   ifdef FLAGS_PROCESSING_DEBUG
#       define __DEBUG_IDX(x) (hlsl::uint)Internal_PathTracerDebugIndex::Idx_##x
#       define FLAG_PROCESS(flag, CanBeCbvValue) eDebug_##flag = 1u << __DEBUG_IDX(flag),
#   else
#       define __FEATURE_IDX(x) (hlsl::uint)Internal_PathTracerFeatureIndex::Idx_##x
#       define FLAG_PROCESS(flag, CanBeCbvValue) eFeature_##flag = 1u << __FEATURE_IDX(flag),
#   endif

#elif defined(FLAGS_PROCESS_AS_STRING)
#   define FLAG_PROCESS(flag, CanBeCbvValue) #flag,

#elif defined(FLAGS_PROCESS_AS_CBCV)
#   define FLAG_PROCESS(flag, CanBeCbvValue) CanBeCbvValue,

#endif

// ================ Start of Structure ================

#if defined(FLAGS_PROCESS_AS_INDEX)
#   ifdef FLAGS_PROCESSING_DEBUG
enum class Internal_PathTracerDebugIndex : hlsl::uint
{
#   else
enum class Internal_PathTracerFeatureIndex : hlsl::uint
{
#   endif

#elif defined(FLAGS_PROCESS_AS_FLAG)
#   ifdef FLAGS_PROCESSING_DEBUG
enum PathTracerDebugFlags : DEBUG_FLAG_TYPE
{
    eDebug_None = 0,
#   else
enum PathTracerFeatureFlags : FEATURE_FLAG_TYPE
{
    eFeature_None = 0,
#   endif

#elif defined(FLAGS_PROCESS_AS_STRING)
#   ifdef FLAGS_PROCESSING_DEBUG
static const char* s_debugFlagNames[DEBUG_COUNT] = {
#   else
static const char* s_featureFlagNames[FEATURE_COUNT] = {
#   endif

#elif defined(FLAGS_PROCESS_AS_CBCV)
#   ifdef FLAGS_PROCESSING_DEBUG
static bool s_canBeCbvValueFlagListDebug[DEBUG_COUNT] = {
#   else
static bool s_canBeCbvValueFlagListFeature[FEATURE_COUNT] = {
#   endif

#endif

// ===================== The List =====================

#ifdef FLAGS_PROCESSING_DEBUG
#   include "PathTracing/Flags/ListDebug.h"
#else
#   include "PathTracing/Flags/ListFeature.h"
#endif

// ================= End of Structure =================

#if defined(FLAGS_PROCESS_AS_INDEX)
#   ifdef FLAGS_PROCESSING_DEBUG
    __INTERNAL_DEBUG_COUNT
#   else
    __INTERNAL_FEATURE_COUNT
#   endif

#elif defined(FLAGS_PROCESS_AS_FLAG)
#   ifdef FLAGS_PROCESSING_DEBUG
    DEBUG_COUNT = (hlsl::uint)Internal_PathTracerDebugIndex::__INTERNAL_DEBUG_COUNT
#   else
    FEATURE_COUNT = (hlsl::uint)Internal_PathTracerFeatureIndex::__INTERNAL_FEATURE_COUNT
#   endif

#endif
};

// ================== Mark Completed ==================

#if defined(FLAGS_PROCESS_AS_INDEX)
#   ifdef FLAGS_PROCESSING_DEBUG
#       define FLAGS_PROCESS_AS_INDEX_COMPLETED_DEBUG
#   else
#       define FLAGS_PROCESS_AS_INDEX_COMPLETED
#   endif

#elif defined(FLAGS_PROCESS_AS_FLAG)
#   ifdef FLAGS_PROCESSING_DEBUG
#       define FLAGS_PROCESS_AS_FLAG_COMPLETED_DEBUG
#   else
#       define FLAGS_PROCESS_AS_FLAG_COMPLETED
#   endif

#elif defined(FLAGS_PROCESS_AS_STRING)
#   ifdef FLAGS_PROCESSING_DEBUG
#       define FLAGS_PROCESS_AS_STRING_COMPLETED_DEBUG
#   else
#       define FLAGS_PROCESS_AS_STRING_COMPLETED
#   endif

#elif defined(FLAGS_PROCESS_AS_CBCV)
#   ifdef FLAGS_PROCESSING_DEBUG
#       define FLAGS_PROCESS_AS_CBCV_COMPLETED_DEBUG
#   else
#       define FLAGS_PROCESS_AS_CBCV_COMPLETED
#   endif

#endif