#ifndef H_FLAGS_METHODS_HLSL_H
#define H_FLAGS_METHODS_HLSL_H

#ifdef __cplusplus
#   error HLSL only
#endif

#include "PathTracing/Flags/Flags.h"

#   ifndef FEATURE_FLAGS
#       define FEATURE_FLAGS 0
#   endif

#   ifndef DEBUG_FLAGS
#       define DEBUG_FLAGS 0
#   endif

#define FEATURE_ENABLED(flag) (FEATURE_FLAGS & FEATURE_FLAG_VALUE_##flag)
#define DEBUG_ENABLED(flag)   (DEBUG_FLAGS & DEBUG_FLAG_VALUE_##flag)

#endif