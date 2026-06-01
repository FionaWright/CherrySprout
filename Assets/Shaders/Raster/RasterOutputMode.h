#ifndef H_RASTEROUTPUTMODE_H
#define H_RASTEROUTPUTMODE_H

enum RasterOutputMode
{
    ePosition,
    eNormalsVertex,
    eNormalsBumped,
    eTangent,
    eBinormal,
    eUV,

    eDirLighting,
    eTex,
    eDirLightingTex,

    eRoughness,
    eMetalness,
    eEmission,

    eViewDir,
    eHalfVec,
    eNDF,
    eFresnel,
    eGeometricMasking,
    eReflection,

    eIrradianceIBL,

    eBsdfSpecular,
    eBsdfDiffuse,
    eBsdfIndirect,
    eBsdfLo,
    eBsdfLoWithIndirect,
};

#endif