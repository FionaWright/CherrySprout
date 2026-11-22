#ifndef H_FEAT_FLAGS_UTILS_H
#define H_FEAT_FLAGS_UTILS_H

#ifndef CONCAT
#   define CONCAT_IMPL(a, b) a##b
#   define CONCAT(a, b) CONCAT_IMPL(a, b)
#endif

#define FEAT_FLAG_TYPE_CORE hlsl::uint
#define FEAT_FLAG_TYPE_ADV hlsl::uint
#define FEAT_FLAG_TYPE_DBG hlsl::uint

#define FLAG_TYPE_IMPL(group) FEAT_FLAG_TYPE_##group
#define FLAG_TYPE(group) FLAG_TYPE_IMPL(group)

#define INCLUDE_FILE_IMPL(group) <PathTracing/FeatFlags/Internal/List##group.h>
#define INCLUDE_FILE(group) INCLUDE_FILE_IMPL(group)

#define ENUM_NAME_INDEX_IMPL(group) Internal_FeatFlagIndices##group
#define ENUM_NAME_INDEX(group) ENUM_NAME_INDEX_IMPL(group)

#define ENUM_NAME_FLAG_IMPL(group) FeatFlags##group
#define ENUM_NAME_FLAG(group) ENUM_NAME_FLAG_IMPL(group)

#define ENUM_VALUE_NONE_NAME_IMPL(group) e##group##_None
#define ENUM_VALUE_NONE_NAME(group) ENUM_VALUE_NONE_NAME_IMPL(group)

#define ENUM_VALUE_NAME_IMPL(group, flag) e##group##_##flag
#define ENUM_VALUE_NAME(group, flag) ENUM_VALUE_NAME_IMPL(group, flag)

#define FLAG_INDEX_IMPL(group, flag) ( (hlsl::uint) Internal_FeatFlagIndices##group ::Idx_##flag )
#define FLAG_INDEX(group, flag) FLAG_INDEX_IMPL(group, flag)

#endif