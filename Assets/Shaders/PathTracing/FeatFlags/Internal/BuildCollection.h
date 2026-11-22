// ReSharper disable once CppMissingIncludeGuard

#define FLAGS_PROCESS_AS_INDEX
#   include "PathTracing/FeatFlags/Internal/Processing.h"
#undef FLAGS_PROCESS_AS_INDEX

#define FLAGS_PROCESS_AS_FLAG
#   include "PathTracing/FeatFlags/Internal/Processing.h"
#undef FLAGS_PROCESS_AS_FLAG

#define FLAGS_PROCESS_AS_IS_QS
#   include "PathTracing/FeatFlags/Internal/Processing.h"
#undef FLAGS_PROCESS_AS_IS_QS

#ifdef __cplusplus
#   define FLAGS_PROCESS_AS_STRING
#       include "PathTracing/FeatFlags/Internal/Processing.h"
#   undef FLAGS_PROCESS_AS_STRING
#endif