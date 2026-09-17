#ifndef H_MM_ENUM_H
#define H_MM_ENUM_H

#include "Utils/HlslGlue.h"

#define MICROFACET_GGX_SMITH_VNDF_ANISO 0
#define MICROFACET_GGX_SMITH_VNDF       1
#define MICROFACET_GGX_VCAVITY_VNDF     2
#define MICROFACET_GGX_SMITH_ANISO      3
#define MICROFACET_GGX_SMITH            4
#define MICROFACET_BECKMANN_SMITH       5

enum class MicrofacetModelType : int
{
    eGGX_Smith_VNDF_Aniso = MICROFACET_GGX_SMITH_VNDF_ANISO,
    eGGX_Smith_VNDF = MICROFACET_GGX_SMITH_VNDF,
    eGGX_VCavity_VNDF = MICROFACET_GGX_VCAVITY_VNDF,
    eGGX_Smith_Aniso = MICROFACET_GGX_SMITH_ANISO,
    eGGX_Smith = MICROFACET_GGX_SMITH,
    eBeckmann_Smith = MICROFACET_BECKMANN_SMITH,
    eCount
};

#ifdef __cplusplus

static constexpr MicrofacetModelType s_defaultMMType = MicrofacetModelType::eGGX_Smith;

static const char* s_mmNames[static_cast<hlsl::uint>(MicrofacetModelType::eCount)] =
{
    "GGX-Smith-VNDF-Aniso",
    "GGX-Smith-VNDF",
    "GGX-VCavity-VNDF",
    "GGX-Smith-Aniso",
    "GGX-Smith",
    "Beckmann-Smith"
};

#endif

// Missing Models:
// GGX_VCavity
// GGX_VCavity_Aniso
// GGX_VCavity_VNDF_Aniso
// Beckmann_Smith_Aniso
// Beckmann_Smith_VNDF
// Beckmann_Smith_VNDF_Aniso

#endif