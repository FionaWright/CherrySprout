#ifndef H_EXTRACT_MESH_H
#define H_EXTRACT_MESH_H

#pragma warning(push)
#pragma warning(disable : 4244 4305 4996 4267 4003)

#include "pxr/usd/usd/stage.h"
#include "pxr/usd/usd/primRange.h"
#include "pxr/usd/usdGeom/mesh.h"
#include "pxr/usd/usdGeom/xformCache.h"
#include "pxr/usd/usdGeom/primvarsAPI.h"
#include "pxr/usd/usdGeom/primvar.h"
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

#include "ExtractGeometry.h"
#include "Importer.h"
#include "SceneLoaderUSD.h"

namespace SceneLoaderUSD
{
    inline bool doFlip(const pxr::UsdPrim& prim, const pxr::GfMatrix4d& xform, const pxr::GfMatrix4d& globalXForm)
    {
        const double det = xform.GetDeterminant3();
        const double detGlobal = globalXForm.GetDeterminant3();
        const bool flipFromTransform = (det * detGlobal) < 0;

        pxr::TfToken orientationToken;
        pxr::UsdGeomMesh(prim).GetOrientationAttr().Get(&orientationToken);
        const bool flipFromOrientation = orientationToken == pxr::UsdGeomTokens->leftHanded;;

        const bool flip = flipFromOrientation ^ flipFromTransform;
        return flip;
    }

    inline std::vector<ImporterObject> ExtractMesh(const pxr::UsdPrim& prim, pxr::UsdGeomXformCache& xformCache,
                                      const std::unordered_map<std::string, size_t>& matPathToIdxMap,
                                      const pxr::GfMatrix4d& globalXForm)
    {
        pxr::GfMatrix4d xform = xformCache.GetLocalToWorldTransform(pxr::UsdGeomMesh(prim).GetPrim());
        xform = xform * globalXForm;

        const bool flip = doFlip(prim, xform, globalXForm);

        const ExtractedBuffers buffers = ExtractGeometry(prim, matPathToIdxMap, flip);
        std::vector<ImporterObject> objects;

        for (int i = 0; i < buffers.Indices2D.size(); i++)
        {
            ImporterObject obj;
            obj.Name = prim.GetName().GetString();
            obj.Vertices = buffers.Vertices;
            obj.Indices = buffers.Indices2D[i];
            obj.MaterialIndex = buffers.MaterialIndices[i];

            for (int r = 0; r < 4; r++)
                for (int c = 0; c < 4; c++)
                {
                    const int j = r * 4 + c;
                    obj.M[j] = static_cast<float>(xform[r][c]);
                }

            objects.emplace_back(std::move(obj));
        }

        return objects;
    }

    inline std::vector<ImporterObject> ExtractInstance(const pxr::UsdPrim& prim, pxr::UsdGeomXformCache& xformCache,
                                      const std::unordered_map<std::string, size_t>& matPathToIdxMap,
                                      const pxr::GfMatrix4d& globalXForm)
    {
        std::vector<ImporterObject> objects;

        const pxr::UsdPrim prototype = prim.GetPrototype();

        pxr::GfMatrix4d instanceXForm = xformCache.GetLocalToWorldTransform(pxr::UsdGeomMesh(prim).GetPrim());
        instanceXForm = instanceXForm * globalXForm;

        const bool flip = doFlip(prim, instanceXForm, globalXForm);

        for (const pxr::UsdPrim& childPrim : pxr::UsdPrimRange(prototype))
        {
            if (!childPrim.IsA<pxr::UsdGeomMesh>())
                continue;

            const ExtractedBuffers buffers = ExtractGeometry(prim, matPathToIdxMap, flip);

            for (int i = 0; i < buffers.Indices2D.size(); i++)
            {
                ImporterObject obj;
                obj.Name = prim.GetName().GetString();
                obj.Vertices = buffers.Vertices;
                obj.Indices = buffers.Indices2D[i];
                obj.MaterialIndex = buffers.MaterialIndices[i];

                for (int r = 0; r < 4; r++)
                    for (int c = 0; c < 4; c++)
                    {
                        const int j = r * 4 + c;
                        obj.M[j] = static_cast<float>(instanceXForm[r][c]);
                    }

                objects.emplace_back(std::move(obj));
            }
        }

        return objects;
    }

    inline void ExtractPointInstancer(ImporterContext* ctx, const pxr::UsdStageRefPtr& stage, const pxr::UsdPrim& prim,
                                      pxr::UsdGeomXformCache& xformCache,
                                      const std::unordered_map<std::string, size_t>& matPathToIdxMap,
                                      const pxr::GfMatrix4d& globalXForm)
    {
        pxr::UsdGeomPointInstancer instancer(prim);

        pxr::SdfPathVector protoPaths;
        instancer.GetPrototypesRel().GetTargets(&protoPaths);

        pxr::VtArray<pxr::GfVec3f> positions;
        instancer.GetPositionsAttr().Get(&positions);

        pxr::VtArray<pxr::GfQuatf> orientations;
        instancer.GetOrientationsAttr().Get(&orientations);

        pxr::VtArray<pxr::GfVec3f> scales;
        instancer.GetScalesAttr().Get(&scales);

        pxr::VtArray<int> indices;
        instancer.GetProtoIndicesAttr().Get(&indices);

        std::vector<std::vector<ImporterObject>> protoObjects;
        for (const auto& path : protoPaths)
        {
            pxr::UsdPrim protoPrim = stage->GetPrimAtPath(path);

            std::vector<ImporterObject> objects;
            for (const auto& childPrim : pxr::UsdPrimRange(protoPrim))
            {
                if (!childPrim.IsA<pxr::UsdGeomMesh>())
                    continue;

                std::vector<ImporterObject> objectsVec = ExtractMesh(childPrim, xformCache, matPathToIdxMap, globalXForm);
                for (int i = 0; i < objectsVec.size(); i++)
                    objects.emplace_back(std::move(objectsVec[i]));
            }

            protoObjects.emplace_back(std::move(objects));
        }

        pxr::GfMatrix4d instancerXForm = xformCache.GetLocalToWorldTransform(prim);

        const size_t instanceCount = positions.size();
        for (size_t i = 0; i < instanceCount; i++)
        {
            const int protoIdx = indices[i];
            if (protoIdx < 0 || protoIdx >= protoObjects.size())
                continue;

            pxr::GfMatrix4d T(1), R(1), S(1);
            T.SetTranslate(pxr::GfVec3d(positions[i]));

            if (i < orientations.size())
                R.SetRotate(pxr::GfMatrix3d(orientations[i]));

            if (i < scales.size())
                S.SetScale(pxr::GfVec3d(scales[i]));

            pxr::GfMatrix4d instanceXForm = S * R * T;
            instanceXForm = instanceXForm * instancerXForm;

            instanceXForm = instanceXForm * globalXForm;

            for (const auto& obj : protoObjects[protoIdx])
            {
                ImporterObject instancedObj = obj;

                for (int r = 0; r < 4; r++)
                    for (int c = 0; c < 4; c++)
                    {
                        const int rc = r * 4 + c;
                        instancedObj.M[rc] = static_cast<float>(instanceXForm[r][c]);
                    }

                ctx->Objects.emplace_back(std::move(instancedObj));
            }
        }
    }
}

#endif
