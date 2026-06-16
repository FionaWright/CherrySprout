#ifndef H_DEBUG_ID_H
#define H_DEBUG_ID_H

#ifndef __cplusplus

// HLSL:
// Fills an enum

#   define CREATE_ID(id, _) id,

enum DebugIdEnum : uint
{

#else

// C++:
// Fills a string vector

#   define CREATE_ID(_, string) string,

static const char* s_debugIdList[] = {

#endif

    CREATE_ID(NO_METAL_GLASS            , "Transmissive material is metal")
    CREATE_ID(NO_EMISSIVE_GLASS         , "Transmissive material is emissive")

    CREATE_ID(BxDF_PBR_GLASS_SPEC_F     , "BxDF_PBR Trans Specular Lobe: f invalid")

    CREATE_ID(CDF_SQUARE_ENV_MAP        , "Equal-Area EnvMap must be square")

};

#endif
