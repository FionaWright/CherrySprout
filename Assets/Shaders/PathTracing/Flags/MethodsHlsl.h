#ifndef H_FLAGS_METHODS_HLSL_H
#define H_FLAGS_METHODS_HLSL_H

#ifdef __cplusplus
#   error HLSL only
#endif

#include "PathTracing/Flags/Flags.h"

#   ifndef FEATURE_FLAGS
#       define FEATURE_FLAGS eFeature_None
#   endif

#   ifndef DEBUG_FLAGS
#       define DEBUG_FLAGS eDebug_None
#   endif

#define FEATURE_ENABLED(flag)  (FEATURE_FLAGS   &   eFeature_##flag)
#define DEBUG_ENABLED(flag)    (DEBUG_FLAGS     &   eDebug_##flag)

#define FEATURE_ENABLED_PREPROC(flag) (FEATURE_FLAG_##flag)

#endif