// ReSharper disable once CppMissingIncludeGuard
#include "Utils/HlslGlue.h"

#ifndef FLAG_GROUP
#error "Flag Group not defined"
#define FLAG_GROUP CORE // For Linter
#endif

// ================ FLAG_PROCESS Macro ================

#ifdef FLAG_PROCESS
#   undef FLAG_PROCESS
#endif

#if defined(FLAGS_PROCESS_AS_INDEX)
#   define FLAG_PROCESS(flag, isQuickSwitchable) Idx_##flag,

#elif defined(FLAGS_PROCESS_AS_FLAG)
#   define FLAG_PROCESS(flag, isQuickSwitchable) ENUM_VALUE_NAME(FLAG_GROUP, flag) = (FLAG_TYPE(FLAG_GROUP))(1) << FLAG_INDEX(FLAG_GROUP, flag),

#elif defined(FLAGS_PROCESS_AS_STRING)
#   define FLAG_PROCESS(flag, isQuickSwitchable) #flag,

#elif defined(FLAGS_PROCESS_AS_IS_QS)
#   define FLAG_PROCESS(flag, isQuickSwitchable) isQuickSwitchable,

#endif

// ================ Start of Structure ================

#if defined(FLAGS_PROCESS_AS_INDEX)
enum class ENUM_NAME_INDEX(FLAG_GROUP) : hlsl::uint
{

#elif defined(FLAGS_PROCESS_AS_FLAG)
enum ENUM_NAME_FLAG(FLAG_GROUP) : CONCAT(FEAT_FLAG_TYPE_, FLAG_GROUP)
{
    ENUM_VALUE_NONE_NAME(FLAG_GROUP) = 0,

#elif defined(FLAGS_PROCESS_AS_STRING)
static const char* CONCAT(s_featFlagNames, FLAG_GROUP)[CONCAT(FEAT_FLAG_COUNT_, FLAG_GROUP)] = {

#elif defined(FLAGS_PROCESS_AS_IS_QS)
static bool CONCAT(s_featFlagsIsQs, FLAG_GROUP)[CONCAT(FEAT_FLAG_COUNT_, FLAG_GROUP)] = {

#endif

// ===================== The List =====================

#include INCLUDE_FILE(FLAG_GROUP)

// ================= End of Structure =================

#if defined(FLAGS_PROCESS_AS_INDEX)
    CONCAT(__INTERNAL_FEAT_FLAG_COUNT_, FLAG_GROUP)

#elif defined(FLAGS_PROCESS_AS_FLAG)
    CONCAT(FEAT_FLAG_COUNT_, FLAG_GROUP) = (hlsl::uint) CONCAT(Internal_FeatFlagIndices, FLAG_GROUP) :: CONCAT(__INTERNAL_FEAT_FLAG_COUNT_, FLAG_GROUP)

#endif
};