#ifndef H_CONSTANTS_H
#define H_CONSTANTS_H

#include "Utils/Constants.h"

// ============ DATA ===========

#ifndef __cplusplus

#define UINT_MAX            4294967296.0

#define NAN                 asfloat(0x7FC00000)

#define INF                 asfloat(0x7F800000)

#define NEG_INF             asfloat(0xFF800000)

#endif

// ============ MATH ===========

#define PI                  3.141592653589793

#define EPSILON             1e-6

#define ONE_MINUS_EPSILON   0x1.fffffep-1

// ============ PHYSICS ========

#define IOR_AIR             CreateComplex(1.0f, 0.0f)

#define BOLTZMANN           1.38064852e-23f

#define PLANK               6.62607015e-34f

#define SPEED_OF_LIGHT      299792458.0f // TODO: What units?

// ============ COLORS ===========

#define COLOR_RED       float3(1,0,0)

#define COLOR_GREEN     float3(0,1,0)

#define COLOR_BLUE      float3(0,0,1)

#define COLOR_WHITE     float3(1,1,1)

#define COLOR_BLACK     float3(0,0,0)

#endif