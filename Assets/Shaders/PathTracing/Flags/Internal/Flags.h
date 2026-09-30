#ifndef H_FLAGS_FLAGS_H
#define H_FLAGS_FLAGS_H

// ==================== Feature ====================

#define FLAGS_PROCESS_AS_INDEX
#   include "PathTracing/Flags/Internal/Processing.h"
#undef FLAGS_PROCESS_AS_INDEX

#define FLAGS_PROCESS_AS_FLAG
#   include "PathTracing/Flags/Internal/Processing.h"
#undef FLAGS_PROCESS_AS_FLAG

#define FLAGS_PROCESS_AS_CBCV
#   include "PathTracing/Flags/Internal/Processing.h"
#undef FLAGS_PROCESS_AS_CBCV

#ifdef __cplusplus
#   define FLAGS_PROCESS_AS_STRING
#       include "PathTracing/Flags/Internal/Processing.h"
#   undef FLAGS_PROCESS_AS_STRING
#endif

// ==================== Debug ====================

#define FLAGS_PROCESSING_DEBUG

#define FLAGS_PROCESS_AS_INDEX
#   include "PathTracing/Flags/Internal/Processing.h"
#undef FLAGS_PROCESS_AS_INDEX

#define FLAGS_PROCESS_AS_FLAG
#   include "PathTracing/Flags/Internal/Processing.h"
#undef FLAGS_PROCESS_AS_FLAG

#define FLAGS_PROCESS_AS_CBCV
#   include "PathTracing/Flags/Internal/Processing.h"
#undef FLAGS_PROCESS_AS_CBCV

#ifdef __cplusplus
#   define FLAGS_PROCESS_AS_STRING
#       include "PathTracing/Flags/Internal/Processing.h"
#   undef FLAGS_PROCESS_AS_STRING
#endif

#endif