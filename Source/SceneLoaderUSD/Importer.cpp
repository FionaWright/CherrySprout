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
#include "pxr/usd/usdLux/lightAPI.h"
#include "pxr/usd/usdShade/material.h"
#include "pxr/usd/usdShade/materialBindingAPI.h"
#include "pxr/base/gf/matrix4d.h"
#include "pxr/base/plug/registry.h"
#include "pxr/base/plug/plugin.h"

#pragma warning(pop)

#include "Importer.h"

#include <filesystem>
#include <iostream>
#include <ostream>

#include "ExtractLight.h"
#include "Material/ExtractMaterial.h"
#include "ExtractMesh.h"

using namespace SceneLoaderUSD;

ImporterContext SceneLoaderUSD::Import(const char* usdPath, float sceneScale)
{
    std::cout << "Importing USD: " << usdPath << std::endl;

    assert(std::filesystem::exists(usdPath));

    ImporterContext context;

    pxr::UsdStageRefPtr stage = pxr::UsdStage::Open(usdPath);
    if (!stage)
        throw std::runtime_error("USD file does not exist");

    const auto& plugins = pxr::PlugRegistry::GetInstance().GetAllPlugins();
    for (auto& plugin : plugins)
    {
        std::cout << "USD Plugin Registered: " << plugin->GetName() << std::endl;
    }

    context.Materials.emplace_back(); // Assign default fallback material for objects missing materials

    std::unordered_map<std::string, size_t> matPathToIdxMap;
    for (const pxr::UsdPrim& prim : stage->Traverse())
    {
        if (prim.IsA<pxr::UsdShadeMaterial>())
        {
            matPathToIdxMap[prim.GetPath().GetString()] = context.Materials.size();
            const Material mat = ExtractMaterial(pxr::UsdShadeMaterial(prim), context.TextureFilePaths);
            context.Materials.emplace_back(mat);

            std::cout << "Imported Material: " << prim.GetPath().GetString() << std::endl;
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

    pxr::GfMatrix4d ToYUp(1.0), SceneScale(1.0);
    SceneScale.SetScale(sceneScale);

    const pxr::TfToken upAxis = pxr::UsdGeomGetStageUpAxis(stage);
    if (upAxis == pxr::UsdGeomTokens->z)
    {
        pxr::GfRotation rot(pxr::GfVec3d(1, 0, 0), -90.0f);
        ToYUp.SetRotate(rot);

        std::cout << "Scene is Z-up" << std::endl;
    }

    pxr::GfMatrix4d RhToLh(1.0);
    RhToLh.SetDiagonal(pxr::GfVec4d(1, 1, -1, 1));

    pxr::GfMatrix4d globalXform = SceneScale * ToYUp * RhToLh;

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
            std::cout << "Extracted Point Instancer: " << prim.GetName().GetString() << std::endl;
            continue;
        }

        if (prim.IsInPrototype() || prim.IsInstanceProxy())
            continue;

        if (prim.IsInstance())
        {
            std::vector<ImporterObject> objects = ExtractInstance(prim, xformCache, matPathToIdxMap, globalXform);
            for (auto & object : objects)
            {
                context.Objects.emplace_back(std::move(object));
                std::cout << "Extracted Instance: " << object.Name << std::endl;
            }
            continue;
        }

        if (prim.IsA<pxr::UsdGeomMesh>())
        {
            std::vector<ImporterObject> objects = ExtractMesh(prim, xformCache, matPathToIdxMap, globalXform);
            for (int i = 0; i < objects.size(); ++i)
            {
                std::cout << "Extracted Mesh: " << objects[i].Name << std::endl;
                context.Objects.emplace_back(std::move(objects[i]));
            }
            continue;
        }

        if (prim.HasAPI<pxr::UsdLuxLightAPI>())
        {
            PunctualLight light = ExtractLight(prim, xformCache, globalXform);
            context.PunctualLights.emplace_back(light);
            std::cout << "Extracted Light: " << static_cast<int>(light.Type) << std::endl;
            continue;
        }
    }

    std::cout << "Importer finished" << std::endl;

    return context;
}
