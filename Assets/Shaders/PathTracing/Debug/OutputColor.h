#ifndef H_DBG_OUTPUT_COLOR_H
#define H_DBG_OUTPUT_COLOR_H

#include "Utils/HlslGlue.h"
#include "PathTracing/Flags.h"

enum class DebugOutputIndex : hlsl::uint
{
    eDebugOutput_Disabled,

    // Core:
    eDebugOutput_RNG,

    // GetHitInfo:
    eDebugOutput_InstanceIdx,
    eDebugOutput_MaterialIdx,
    eDebugOutput_Barycentrics,
    eDebugOutput_NormalShadedFF,
    eDebugOutput_NormalGeometricFF,
    eDebugOutput_Tangent,
    eDebugOutput_Bitangent,
    eDebugOutput_UV,

    // Textures:
    eDebugOutput_TexAlbedo,
    eDebugOutput_TexNormal,
    eDebugOutput_TexEmissive,
    eDebugOutput_TexRoughness,
    eDebugOutput_TexMetallic,

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
    eDebugOutput_H_s,
    eDebugOutput_H_w,
    eDebugOutput_L_s,
    eDebugOutput_L_w,
    eDebugOutput_Alpha,
    eDebugOutput_D,
    eDebugOutput_G,
    eDebugOutput_F,
    eDebugOutput_f,
    eDebugOutput_PDF,

    // NEE:
    eDebugOutput_NEE_L_w,
    eDebugOutput_NEE_PDF,
    eDebugOutput_NEE_Occluded,
    eDebugOutput_NEE_L_Direct,
    eDebugOutput_NEE_MIS_Weight,

    eCount
};

#ifdef __cplusplus

static const char* s_debugOutputIdxNames[static_cast<hlsl::uint>(DebugOutputIndex::eCount)] =
{
    "DISABLED",

    // Core
    "RNG",

    // GetHitInfo
    "Instance Index",
    "Material Index",
    "Barycentrics",
    "Normal Shaded FF",
    "Normal Geometric FF",
    "Tangent",
    "Bitangent",
    "UV",

    // Textures:
    "Tex Albedo",
    "Tex Normal",
    "Tex Emissive",
    "Tex Roughness",
    "Tex Metallic",

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
    "H_s",
    "H_w",
    "L_s",
    "L_w",
    "Alpha",
    "D",
    "G",
    "F",
    "f",
    "PDF",

    // NEE
    "NEE L_w",
    "NEE PDF",
    "NEE Occluded",
    "NEE L_Direct",
    "NEE MIS Weight",
};

static constexpr DebugOutputIndex s_defaultOutputIndex = DebugOutputIndex::eDebugOutput_Disabled;

#elif defined(DEBUG_OUTPUT_COLOR)

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

#    define DBG_FORCE_OUTPUT3(value) { gDebugValueFound = true; gDebugValue = value; return; }

#    define DBG_OUTPUT_SET(output) { output = gDebugValue; }

#else
#    define DBG_OUTPUT3(value, idx)
#    define DBG_OUTPUT2(value, idx)
#    define DBG_OUTPUT1(value, idx)
#    define DBG_FORCE_OUTPUT3(value)
#    define DBG_OUTPUT_SET(output)

#endif

#endif