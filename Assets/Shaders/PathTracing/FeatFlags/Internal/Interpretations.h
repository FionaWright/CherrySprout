#ifndef H_FLAGS_FLAGS_H
#define H_FLAGS_FLAGS_H

#include "PathTracing/FeatFlags/Internal/Utils.hlsli"

// ==================== Core ====================

#define FLAG_GROUP CORE
#include "PathTracing/FeatFlags/Internal/BuildCollection.h"

// ==================== Debug ====================

#undef FLAG_GROUP
#define FLAG_GROUP DBG
#include "PathTracing/FeatFlags/Internal/BuildCollection.h"

// ==================== Advanced ====================

#undef FLAG_GROUP
#define FLAG_GROUP ADV
#include "PathTracing/FeatFlags/Internal/BuildCollection.h"

#endif



