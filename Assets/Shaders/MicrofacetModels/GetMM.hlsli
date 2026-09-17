#ifndef H_MICRO_CLASS_DEF_H
#define H_MICRO_CLASS_DEF_H

#include "MicrofacetModels/MMTypes.h"

struct MicrofacetModel;

#ifndef MICROFACET_MODEL_TYPE
#    define MICROFACET_MODEL_TYPE MICROFACET_GGX_SMITH
#endif

#if MICROFACET_MODEL_TYPE == MICROFACET_GGX_SMITH_VNDF_ANISO
#    include "MicrofacetModels/MM_GGX_Smith_Aniso_VNDF.hlsli"

#elif MICROFACET_MODEL_TYPE == MICROFACET_GGX_SMITH_VNDF
#    include "MicrofacetModels/MM_GGX_Smith_Iso_VNDF.hlsli"

#elif MICROFACET_MODEL_TYPE == MICROFACET_GGX_VCAVITY_VNDF
#    include "MicrofacetModels/MM_GGX_VCavity_Iso_VNDF.hlsli"

#elif MICROFACET_MODEL_TYPE == MICROFACET_GGX_SMITH_ANISO
#    include "MicrofacetModels/MM_GGX_Smith_Aniso.hlsli"

#elif MICROFACET_MODEL_TYPE == MICROFACET_GGX_SMITH
#    include "MicrofacetModels/MM_GGX_Smith_Iso.hlsli"

#elif MICROFACET_MODEL_TYPE == MICROFACET_BECKMANN_SMITH
#    include "MicrofacetModels/MM_Beckmann_Smith_Iso.hlsli"

#endif

#endif