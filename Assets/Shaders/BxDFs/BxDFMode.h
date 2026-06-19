#ifndef H_BXDF_MODE_H
#define H_BXDF_MODE_H

#include "Utils/HlslGlue.h"

#ifdef __cplusplus

enum class BxdfMode : hlsl::uint
{
    eLambertian,
    ePBR,
    ePrincipled,

    eCount
};

static const char* s_bxdfNames[static_cast<hlsl::uint>(BxdfMode::eCount)] =
{
    "Lambertian",
    "PBR",
    "Principled"
};

static constexpr BxdfMode s_defaultBxdfMode = BxdfMode::eLambertian;

#else

#define BXDF_LAMBERTIAN     0
#define BXDF_PBR            1
#define BXDF_PRINCIPLED     2

#   ifndef BXDF_MODE
#       define BXDF_MODE BXDF_LAMBERTIAN
#   endif

#define BXDF_ENABLED(mode)  BXDF_MODE == BXDF_##mode

#endif

#endif