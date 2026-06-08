#ifndef H_DBG_OUTPUT_COLOR_H
#define H_DBG_OUTPUT_COLOR_H

#include "Utils/HlslGlue.h"
#include "PathTracing/Flags.h"

enum class DebugOutputIndex : hlsl::uint
{
    // Core:
    eDebugOutput_RNG,

    // GetHitInfo:
    eDebugOutput_InstanceIdx,
    eDebugOutput_MaterialIdx,
    eDebugOutput_Barycentrics,
    eDebugOutput_NormalMap,
    eDebugOutput_NormalShadedFF,
    eDebugOutput_NormalGeometricFF,
    eDebugOutput_UV,

    // Material:
    eDebugOutput_Albedo,
    eDebugOutput_Opacity,
    eDebugOutput_EmissiveStrength,
    eDebugOutput_EmissiveColor,
    eDebugOutput_Emission,
    eDebugOutput_TransmissionFactor,
    eDebugOutput_TransmissionColor,
    eDebugOutput_Roughness,
    eDebugOutput_Metallic,
    eDebugOutput_SpecularFactor,
    eDebugOutput_AnisoStrength,
    eDebugOutput_IorN,

    // BSDF:
    eDebugOutput_Hs,
    eDebugOutput_Hw,
    eDebugOutput_Ls,
    eDebugOutput_Lw,
    eDebugOutput_Alpha,
    eDebugOutput_D,
    eDebugOutput_G,
    eDebugOutput_F,
    eDebugOutput_f,
    eDebugOutput_PDF,

    eCount
};

#ifdef __cplusplus

static const char* s_debugOutputIdxNames[static_cast<hlsl::uint>(DebugOutputIndex::eCount)] =
{
    // Core
    "RNG",

    // GetHitInfo
    "Instance Index",
    "Material Index",
    "Barycentrics",
    "Normal Map",
    "Normal Shaded FF",
    "Normal Geometric FF",
    "UV",

    // Material
    "Albedo",
    "Opacity",
    "Emissive Strength",
    "Emissive Color",
    "Emission (Li)",
    "Transmission Factor",
    "Transmission Color",
    "Roughness",
    "Metallic",
    "Specular Factor",
    "Anisotropy Strength",
    "IOR N",

    // BSDF
    "Hs",
    "Hw",
    "Ls",
    "Lw",
    "Alpha",
    "D",
    "G",
    "F",
    "f",
    "PDF",
};

static constexpr DebugOutputIndex s_defaultOutputIndex = DebugOutputIndex::eDebugOutput_Albedo;

//#elif DEBUG_ENABLED(OutputColor)
#else

static bool gDebugValueFound = false;
static float3 gDebugValue = NAN;

#    ifndef DEBUG_OUTPUT_COLOR
#        define DEBUG_OUTPUT_COLOR DebugOutputIndex::eDebugOutput_Albedo
#    endif

#    define DBG_OUTPUT3(value, idx)                                                                                                          \
{                                                                                                                                            \
    if (!gDebugValueFound && DebugOutputIndex::eDebugOutput_##idx == (DebugOutputIndex)(DEBUG_OUTPUT_COLOR))                                 \
    {                                                                                                                                        \
        gDebugValueFound = true;                                                                                                             \
        gDebugValue = value;                                                                                                           \
    }                                                                                                                                        \
}                                                                                                                                            \

#    define DBG_OUTPUT2(value, idx) DBG_OUTPUT3(float3(value, 0), idx)
#    define DBG_OUTPUT1(value, idx) DBG_OUTPUT3(value.xxx, idx)

#    define DBG_OUTPUT3_FORCE(value) { gDebugValueFound = true; gDebugValue = value; return; }

#    define DBG_OUTPUT_SET(output) { output = gDebugValue; }

//#else
//#    define DBG_OUTPUT_START()
//#    define DBG_OUTPUT3(value, idx)
//#    define DBG_OUTPUT2(value, idx)
//#    define DBG_OUTPUT1(value, idx)
//#    define DBG_OUTPUT3_FORCE(value)
//#    define DBG_OUTPUT_SET(output)

#endif

#endif