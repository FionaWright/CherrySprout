//
// Created by fionaw on 30/05/2026.
//

#pragma warning(push)
#pragma warning(disable : 4244 4305 4996 4267 4003)

#include "pxr/usd/usd/stage.h"
#include "pxr/usd/usd/primRange.h"
#include "pxr/usd/usdGeom/mesh.h"
#include "pxr/usd/usdGeom/xformCache.h"
#include "pxr/usd/usdGeom/pointInstancer.h"
#include "pxr/usd/usdGeom/metrics.h"
#include "pxr/usd/usdLux/lightAPI.h"
#include "pxr/usd/usdLux/distantLight.h"
#include "pxr/usd/usdShade/shader.h"
#include "pxr/usd/usdShade/tokens.h"
#include "pxr/usd/usdShade/material.h"
#include "pxr/usd/usdShade/materialBindingAPI.h"
#include "pxr/base/gf/matrix4d.h"
#include "pxr/base/gf/rotation.h"
#include "pxr/base/gf/vec3f.h"
#include "pxr/base/tf/token.h"
#include "pxr/usd/sdf/assetPath.h"

#pragma warning(pop)

#include "Importer.h"

#include <iostream>
#include <ostream>

#include "../../Headers/SceneLoaderUSD/Material/ExtractMaterial.h"
#include "ExtractMesh.h"

using namespace SceneLoaderUSD;

ImporterContext SceneLoaderUSD::Import(const char* usdPath, float sceneScale)
{
    std::cout << "Importing USD: " << usdPath << std::endl;

    ImporterContext context;

    pxr::UsdStageRefPtr stage = pxr::UsdStage::Open(usdPath);
    if (!stage)
        throw std::runtime_error("USD file does not exist");

    std::unordered_map<std::string, size_t> matPathToIdxMap;
    for (const pxr::UsdPrim& prim : stage->Traverse())
    {
        if (prim.IsA<pxr::UsdShadeMaterial>())
        {
            matPathToIdxMap[prim.GetPath().GetString()] = context.Materials.size();
            const Material mat = ExtractMaterial(pxr::UsdShadeMaterial(prim), context.TextureFilePaths);
            context.Materials.emplace_back(mat);
        }
    }

    pxr::UsdGeomXformCache xformCache(pxr::UsdTimeCode::Default());

    std::unordered_set<pxr::SdfPath, pxr::SdfPath::Hash> prototypePaths;

    for (const pxr::UsdPrim& prim : stage->Traverse())
    {
        if (!prim.IsA<pxr::UsdGeomPointInstancer>())
            continue;

        pxr::UsdGeomPointInstancer instancer(prim);
        pxr::UsdRelationship relationship = instancer.GetPrototypesRel();
        pxr::SdfPathVector targets;
        relationship.GetTargets(&targets);

        for (const auto& path : targets)
        {
            prototypePaths.insert(path);
        }
    }

    pxr::GfMatrix4d globalXform;
    globalXform.SetScale(sceneScale);

    const pxr::Usd_PrimFlagsPredicate predicate = pxr::UsdPrimIsActive && pxr::UsdPrimIsDefined && !pxr::UsdPrimIsAbstract;
    for (const pxr::UsdPrim& prim : stage->Traverse(predicate))
    {
        if (prim.IsA<pxr::UsdShadeMaterial>())
            continue;

        bool isPrototype = false;
        for (const auto& protoPath : prototypePaths)
        {
            if (prim.GetPath().HasPrefix(protoPath))
            {
                isPrototype = true;
                break;
            }
        }
        if (isPrototype)
            continue;

        if (prim.IsA<pxr::UsdGeomPointInstancer>())
        {
            ExtractPointInstancer(&context, stage, prim, xformCache, matPathToIdxMap, globalXform);
            continue;
        }

        if (prim.IsInPrototype() || prim.IsInstanceProxy())
            continue;

        if (prim.IsA<pxr::UsdGeomMesh>())
        {
            ImporterObject obj = ExtractMesh(prim, xformCache, matPathToIdxMap, globalXform);
            context.Objects.emplace_back(std::move(obj));
            continue;
        }

        if (prim.HasAPI<pxr::UsdLuxLightAPI>())
        {
            // TODO
            continue;
        }
    }

    std::cout << "Importer finished" << std::endl;

    return context;
}
