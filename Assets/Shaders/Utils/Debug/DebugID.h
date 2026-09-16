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

    // TODO: Use a single param and convert MY_DEBUG_ID -> "My Debug Id" ?

    CREATE_ID(NO_METAL_GLASS            , "Transmissive material is metal")
    CREATE_ID(NO_EMISSIVE_GLASS         , "Transmissive material is emissive")

    CREATE_ID(BxDF_PBR_H_S     , "BxDF_PBR H_s invalid")

    CREATE_ID(BxDF_PBR_GLASS_SPEC_F     , "BxDF_PBR Trans Specular Lobe: f invalid")
    CREATE_ID(BxDF_PBR_GLASS_SPEC_PDF   , "BxDF_PBR Trans Specular Lobe: pdf invalid")
    CREATE_ID(BxDF_PBR_GLASS_SPEC_L_S   , "BxDF_PBR Trans Specular Lobe: L_s invalid")

    CREATE_ID(BxDF_PBR_GLASS_REFRACT_F     , "BxDF_PBR Trans Refract Lobe: f invalid")
    CREATE_ID(BxDF_PBR_GLASS_REFRACT_PDF   , "BxDF_PBR Trans Refract Lobe: pdf invalid")
    CREATE_ID(BxDF_PBR_GLASS_REFRACT_L_S   , "BxDF_PBR Trans Refract Lobe: L_s invalid")

    CREATE_ID(BxDF_PBR_SPEC_F   , "BxDF_PBR Specular Lobe: f invalid")
    CREATE_ID(BxDF_PBR_SPEC_PDF   , "BxDF_PBR Specular Lobe: pdf invalid")
    CREATE_ID(BxDF_PBR_SPEC_L_S   , "BxDF_PBR Specular Lobe: L_s invalid")

    CREATE_ID(BxDF_PBR_DIFF_F   , "BxDF_PBR Diffuse Lobe: f invalid")
    CREATE_ID(BxDF_PBR_Diff_PDF   , "BxDF_PBR Diffuse Lobe: pdf invalid")
    CREATE_ID(BxDF_PBR_Diff_L_S   , "BxDF_PBR Diffuse Lobe: L_s invalid")

    CREATE_ID(CDF_SQUARE_ENV_MAP        , "Equal-Area EnvMap must be square")
    CREATE_ID(CDF_END_IN_ONE            , "CDF must end in one")

    CREATE_ID(REVALUATE_F   , "Revaluate Failed: f")
    CREATE_ID(REVALUATE_PDF   , "Revaluate Failed: pdf")
    CREATE_ID(COLOR_OUT   , "Output Color Invalid")

    CREATE_ID(UNNORMALIZED_VECTOR   , "Vector is unnormalized")
    CREATE_ID(NON_POSITIVE_PDF   , "PDF is not positive or zero")

    CREATE_ID(UNSUPPORTED_LIGHT_TYPE   , "Unsupported light type")
    CREATE_ID(DISABLED_LIGHT_INDEX   , "Light index was chosen that isn't enabled")
    CREATE_ID(OOB_LIGHT_INDEX   , "Light index was chosen out of bounds")

    CREATE_ID(RESTIR_W_Y_RANGE   , "ReSTIR: W_Y incorrect value")
    CREATE_ID(RESTIR_WEIGHT_SUM_RANGE   , "ReSTIR: WeightSum incorrect value")
    CREATE_ID(RESTIR_CONF_RANGE   , "ReSTIR: Confidence incorrect value")
    CREATE_ID(RESTIR_DIR_NORM   , "ReSTIR: Direction unnormalized")

    CREATE_ID(GD_INVALID_VERTEX_TYPE   , "GD: Invalid vertex type")

};

#endif
