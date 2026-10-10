// ReSharper disable once CppMissingIncludeGuard
#include "Utils/HlslGlue.h"

#define FEAT_FLAG_TYPE_CORE hlsl::uint
#define FEAT_FLAG_TYPE_DBG hlsl::uint

// ====================== Guards ======================

#if defined(FLAGS_PROCESS_AS_INDEX)

#elif defined(FLAGS_PROCESS_AS_FLAG)
#   ifdef FLAGS_PROCESSING_DEBUG
#       ifndef FLAGS_PROCESS_AS_INDEX_COMPLETED_DEBUG
#           error You must process this file for indices before processing for flags (Debug)
#       endif
#   else
#       ifndef FLAGS_PROCESS_AS_INDEX_COMPLETED
#           error You must process this file for indices before processing for flags (Core)
#       endif
#   endif

#elif defined(FLAGS_PROCESS_AS_STRING)
#   ifdef FLAGS_PROCESSING_DEBUG
#       ifndef FLAGS_PROCESS_AS_FLAG_COMPLETED_DEBUG
#           error You must process this file for flags before processing for strings (Debug)
#       endif
#   else
#       ifndef FLAGS_PROCESS_AS_FLAG_COMPLETED
#           error You must process this file for flags before processing for strings (Core)
#       endif
#   endif

#elif defined(FLAGS_PROCESS_AS_IS_QS)
#   ifdef FLAGS_PROCESSING_DEBUG
#       ifndef FLAGS_PROCESS_AS_FLAG_COMPLETED_DEBUG
#           error You must process this file for flags before processing for IsQuickSwitchable (Debug)
#       endif
#   else
#       ifndef FLAGS_PROCESS_AS_FLAG_COMPLETED
#           error You must process this file for flags before processing for IsQuickSwitchable (Core)
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
#   define FLAG_PROCESS(flag, isQuickSwitchable) Idx_##flag,

#elif defined(FLAGS_PROCESS_AS_FLAG)
#   ifdef FLAGS_PROCESSING_DEBUG
#       define __IDX_DBG(x) (hlsl::uint)Internal_FeatFlagIndicesDbg::Idx_##x
#       define FLAG_PROCESS(flag, isQuickSwitchable) eDebug_##flag = 1u << __IDX_DBG(flag),
#   else
#       define __IDX_CORE(x) (hlsl::uint)Internal_FeatFlagIndicesCore::Idx_##x
#       define FLAG_PROCESS(flag, isQuickSwitchable) eCore_##flag = 1u << __IDX_CORE(flag),
#   endif

#elif defined(FLAGS_PROCESS_AS_STRING)
#   define FLAG_PROCESS(flag, isQuickSwitchable) #flag,

#elif defined(FLAGS_PROCESS_AS_IS_QS)
#   define FLAG_PROCESS(flag, isQuickSwitchable) isQuickSwitchable,

#endif

// ================ Start of Structure ================

#if defined(FLAGS_PROCESS_AS_INDEX)
#   ifdef FLAGS_PROCESSING_DEBUG
enum class Internal_FeatFlagIndicesDbg : hlsl::uint
{
#   else
enum class Internal_FeatFlagIndicesCore : hlsl::uint
{
#   endif

#elif defined(FLAGS_PROCESS_AS_FLAG)
#   ifdef FLAGS_PROCESSING_DEBUG
enum FeatFlagsDbg : FEAT_FLAG_TYPE_DBG
{
    eDebug_None = 0,
#   else
enum FeatFlagsCore : FEAT_FLAG_TYPE_CORE
{
    eCore_None = 0,
#   endif

#elif defined(FLAGS_PROCESS_AS_STRING)
#   ifdef FLAGS_PROCESSING_DEBUG
static const char* s_featFlagNamesDbg[FEAT_FLAG_COUNT_DBG] = {
#   else
static const char* s_featFlagNamesCore[FEAT_FLAG_COUNT_CORE] = {
#   endif

#elif defined(FLAGS_PROCESS_AS_IS_QS)
#   ifdef FLAGS_PROCESSING_DEBUG
static bool s_featFlagsIsQsDbg[FEAT_FLAG_COUNT_DBG] = {
#   else
static bool s_featFlagsIsQsCore[FEAT_FLAG_COUNT_CORE] = {
#   endif

#endif

// ===================== The List =====================

#ifdef FLAGS_PROCESSING_DEBUG
#   include "PathTracing/FeatFlags/Internal/ListDebug.h"
#else
#include "PathTracing/FeatFlags/Internal/ListCore.h"
#endif

// ================= End of Structure =================

#if defined(FLAGS_PROCESS_AS_INDEX)
#   ifdef FLAGS_PROCESSING_DEBUG
    __INTERNAL_FEAT_FLAG_COUNT_DBG
#   else
    __INTERNAL_FEAT_FLAG_COUNT_CORE
#   endif

#elif defined(FLAGS_PROCESS_AS_FLAG)
#   ifdef FLAGS_PROCESSING_DEBUG
    FEAT_FLAG_COUNT_DBG = (hlsl::uint)Internal_FeatFlagIndicesDbg::__INTERNAL_FEAT_FLAG_COUNT_DBG
#   else
    FEAT_FLAG_COUNT_CORE = (hlsl::uint)Internal_FeatFlagIndicesCore::__INTERNAL_FEAT_FLAG_COUNT_CORE
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

#elif defined(FLAGS_PROCESS_AS_IS_QS)
#   ifdef FLAGS_PROCESSING_DEBUG
#       define FLAGS_PROCESS_AS_IS_QS_COMPLETED_DEBUG
#   else
#       define FLAGS_PROCESS_AS_IS_QS_COMPLETED
#   endif

#endif