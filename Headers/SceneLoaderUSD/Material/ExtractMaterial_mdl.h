#ifndef H_EXTRACT_MATERIAL_MDL_H
#define H_EXTRACT_MATERIAL_MDL_H

#pragma warning(push)
#pragma warning(disable : 4244 4305 4996 4267 4003)

#include "pxr/usd/usd/stage.h"
#include "pxr/usd/usd/primRange.h"
#include "pxr/usd/usdGeom/mesh.h"
#include "pxr/usd/usdLux/lightAPI.h"
#include "pxr/usd/usdShade/shader.h"
#include "pxr/usd/usdShade/tokens.h"
#include "pxr/usd/usdShade/material.h"
#include "pxr/base/gf/vec3f.h"
#include "pxr/base/tf/token.h"
#include "pxr/usd/sdf/assetPath.h"

#pragma warning(pop)

#include "../Importer.h"
#include "Scene/Material.h"

namespace SceneLoaderUSD
{
    // TODO
    inline Material ExtractMaterial_Mdl(const pxr::UsdShadeShader& surface, std::vector<const char*>& textureList)
    {
        Material material{};
        return material;
    }
}

#endif