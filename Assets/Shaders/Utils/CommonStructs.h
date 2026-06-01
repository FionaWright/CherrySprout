#ifndef H_COMMON_STRUCTS_H
#define H_COMMON_STRUCTS_H

#include "Raster/RasterOutputMode.h"
#include "Utils/HlslGlue.h"

struct CbvMatrices
{
    hlsl::float4x4 M, MTI, V, P;
};

struct CbvForward
{
    hlsl::float3 DirLightDir;
    hlsl::uint MaxCubemapMipMaps;

    RasterOutputMode OutputMode;
    hlsl::float3 _;
};

#endif