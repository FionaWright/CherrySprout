#ifndef H_DBG_OUTPUT_COLOR_H
#define H_DBG_OUTPUT_COLOR_H

#include "Utils/HlslGlue.h"

enum class DebugOutputIndex : hlsl::uint
{
    eDebugOutput_Disabled,

    // Core:
    eDebugOutput_RNG,
    eDebugOutput_GBufferMatIdx,
    eDebugOutput_GBufferNormals,
    eDebugOutput_GBufferDepth,
    eDebugOutput_GBufferUv,
    eDebugOutput_GBufferMv,

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
    eDebugOutput_SpecProb,
    eDebugOutput_ReflectProb,

    // NEE:
    eDebugOutput_NEE_L_w,
    eDebugOutput_NEE_PDF,
    eDebugOutput_NEE_Occluded,
    eDebugOutput_NEE_Le,
    eDebugOutput_NEE_MIS_Weight,

    eCount
};

#ifdef __cplusplus

static const char* s_debugOutputIdxNames[static_cast<hlsl::uint>(DebugOutputIndex::eCount)] =
{
    "DISABLED",

    // Core
    "RNG",
    "GBuffer Material Idx",
    "GBuffer Normals",
    "GBuffer Depth",
    "GBuffer UV",
    "GBuffer MV",

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
    "Specular Probability",
    "Reflect Probability",

    // NEE
    "NEE L_w",
    "NEE PDF",
    "NEE Occluded",
    "NEE Le",
    "NEE MIS Weight",
};

static constexpr auto s_defaultOutputIndex = DebugOutputIndex::eDebugOutput_Disabled;

#endif

#endif