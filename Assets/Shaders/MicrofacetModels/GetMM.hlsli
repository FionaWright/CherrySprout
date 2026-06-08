#ifndef H_MICRO_CLASS_DEF_H
#define H_MICRO_CLASS_DEF_H

struct MicrofacetModel;

// Missing:
// VNDF Aniso

#ifndef MICROFACET_MODEL_CHOSEN
#    define NDF_TYPE_GGX
#    define MASKING_SMITH
#endif

// TODO
#if defined(NDF_TYPE_GGX)
#   ifdef ANISOTROPY_ENABLED
#       ifdef SAMPLE_VISIBLE_NORMALS
#           include "MicrofacetModels/MM_GGX_Smith_Aniso_VNDF.hlsli"
#       else
#           include "MicrofacetModels/MM_GGX_Smith_Aniso.hlsli"
#       endif
#   else
#       ifdef SAMPLE_VISIBLE_NORMALS
#           if defined(MASKING_VCAVITY)
#               include "MicrofacetModels/MM_GGX_VCavity_Iso_VNDF.hlsli"
#           elif defined(MASKING_SMITH)
#               include "MicrofacetModels/MM_GGX_Smith_Iso_VNDF.hlsli"
#           endif
#       else
#           include "MicrofacetModels/MM_GGX_Smith_Iso.hlsli"
#       endif
#   endif
#elif defined(NDF_TYPE_BECKMANN)
#    include "MicrofacetModels/MM_Beckmann_Smith_Iso.hlsli"
#endif

void InitializeMM(
    inout MicrofacetModel mm,
    float roughness,
    RngInfo rngInfo, // For computing U3
    float3 V)
{
#ifdef NDF_TYPE_GGX
#   ifdef SAMPLE_VISIBLE_NORMALS
    mm.Init(roughness, rngInfo, V);
#   else
    mm.Init(roughness);
#   endif
#endif
}

void InitializeMMAniso(
    inout MicrofacetModel mm,
    HitInfo hitInfo)
{
#ifdef ANISOTROPY_ENABLED
    mm.InitAniso(hitInfo);
#endif
}

#endif