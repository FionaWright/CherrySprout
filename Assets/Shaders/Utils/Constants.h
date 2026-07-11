#ifndef H_CONSTANTS_H
#define H_CONSTANTS_H

// ============ DATA ===========

#ifndef __cplusplus

#define UINT_MAX            4294967295

#define NAN                 asfloat(0x7FC00000)

#define INF                 asfloat(0x7F800000)

#define NEG_INF             asfloat(0xFF800000)

#endif

// ============ MATH ===========

#define PI                  3.141592653589793

#define TAU                 (2.0f * PI)

#define EPSILON             1e-6

#define ONE_MINUS_EPSILON   0x1.fffffep-1

// ============ PHYSICS ========

#define IOR_N_AIR                                   1.0f

#define BOLTZMANN                                   1.38064852e-23f

#define PLANK                                       6.62607015e-34f

#define SPEED_OF_LIGHT                              299792458.0f // TODO: What units?

#define STANDARD_DAYLIGHT_WAVELENGTH                555.0f // nm

#define LUMINOUS_EFFICACY                           683.0f // lm/W for standard daylight. Used to convert between lm and W

#define CANDELA_TO_RADIANCE                         (LUMINOUS_EFFICACY / STANDARD_DAYLIGHT_WAVELENGTH)

#define CANDELA_TO_RADIANT_INTENSITY                (1.0f / LUMINOUS_EFFICACY) // lm/sr -> W/sr

#define ILLUMINANCE_TO_IRRADIANCE                   (1.0f / LUMINOUS_EFFICACY) // lm/m^2 -> W/m^2

#define SUN_SUBTENDED_SPHERE_COS_ANGULAR_RADIUS     0.9999893f // 0.004625 steradians

#define INV_SUN_SUBTENDED_SPHERE_SOLID_ANGLE        14874.29375f // Assuming same angular radius as above. Used to convert Radiance <-> Radiant Intensity

#define ENV_MAP_IMPORTANCE_RADIANT_INTENSITY        (1e3f / LUMINOUS_EFFICACY) // TODO: Should be used for combined light weights instead of actual total luminance?

// ============ COLORS ===========

#define COLOR_RED       float3(1,0,0)

#define COLOR_GREEN     float3(0,1,0)

#define COLOR_BLUE      float3(0,0,1)

#define COLOR_WHITE     float3(1,1,1)

#define COLOR_BLACK     float3(0,0,0)

#endif