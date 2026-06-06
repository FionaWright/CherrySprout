#ifndef H_EXTRACT_MATERIAL_H
#define H_EXTRACT_MATERIAL_H

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

#include "ExtractMaterial_mdl.h"
#include "Scene/Material.h"

#include "Material/ExtractMaterial_mtlx.h"
#include "Material/ExtractMaterial_simple.h"

namespace SceneLoaderUSD
{
    inline bool TryResolveSurface(const pxr::UsdShadeConnectableAPI& src, pxr::UsdShadeShader& surface)
    {
        const pxr::UsdPrim& prim = src.GetPrim();
        if (prim.IsA<pxr::UsdShadeShader>())
        {
            surface = pxr::UsdShadeShader(prim);
            return true;
        }

        if (prim.IsA<pxr::UsdShadeNodeGraph>())
        {
            const pxr::UsdShadeNodeGraph graph(prim);

            for (const auto& graphInput : graph.GetInputs())
            {
                pxr::UsdShadeConnectableAPI upstream;
                pxr::TfToken name;
                pxr::UsdShadeAttributeType type;

                if (!graphInput.GetConnectedSource(&upstream, &name, &type))
                    continue;

                return TryResolveSurface(upstream, surface);
            }
        }

        return false;
    }

    inline bool SurfaceIsMDL(const pxr::UsdShadeShader& surface)
    {
        pxr::TfToken id;
        surface.GetIdAttr().Get(&id);

        return surface.GetPrim().HasAttribute(pxr::TfToken("info:mdl:sourceAsset")) || id.GetString().find("mdl") != std::string::npos;
    }

    inline Material ExtractMaterial(const pxr::UsdShadeMaterial& mat, std::vector<const char*>& textureList)
    {
        pxr::UsdShadeShader surface;

        // Universal
        {
            auto output = mat.GetSurfaceOutput();
            if (output)
            {
                pxr::UsdShadeConnectableAPI source;
                pxr::TfToken sourceName;
                pxr::UsdShadeAttributeType sourceType;

                if (output.GetConnectedSource(&source, &sourceName, &sourceType))
                {
                    if (!TryResolveSurface(source, surface))
                        return {};

                    if (SurfaceIsMDL(surface))
                        return ExtractMaterial_Mdl(surface, textureList);

                    return ExtractMaterial_Simple(surface, textureList);
                }
            }
        }

        // Contexts
        {
            for (const auto& ctx : {
                pxr::TfToken("ri"),
                pxr::TfToken("arnold"),
                pxr::TfToken("mdl")
            })
            {
                auto output = mat.GetSurfaceOutput(ctx);
                if (!output)
                    continue;

                pxr::UsdShadeConnectableAPI source;
                pxr::TfToken sourceName;
                pxr::UsdShadeAttributeType sourceType;

                if (output.GetConnectedSource(&source, &sourceName, &sourceType))
                {
                    if (!TryResolveSurface(source, surface))
                        return {};

                    if (SurfaceIsMDL(surface))
                        return ExtractMaterial_Mdl(surface, textureList);

                    return ExtractMaterial_Simple(surface, textureList);
                }
            }
        }

        // MaterialX
        {
            pxr::TfToken ctx("mtlx");
            auto output = mat.GetSurfaceOutput(ctx);
            if (output)
            {
                pxr::UsdShadeConnectableAPI source;
                pxr::TfToken sourceName;
                pxr::UsdShadeAttributeType sourceType;

                if (output.GetConnectedSource(&source, &sourceName, &sourceType))
                {
                    if (!TryResolveSurface(source, surface))
                        return {};

                    return ExtractMaterial_MaterialX(surface, textureList);
                }
            }
        }

        throw std::runtime_error("Unsupported material type");
    }
}

#endif