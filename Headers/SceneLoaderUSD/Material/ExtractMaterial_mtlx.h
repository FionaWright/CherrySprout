#ifndef H_EXTRACT_MATERIAL_MTLX_H
#define H_EXTRACT_MATERIAL_MTLX_H

#pragma warning(push)
#pragma warning(disable : 4244 4305 4996 4267 4003)

#include "pxr/usd/usd/stage.h"
#include "pxr/usd/usd/primRange.h"
#include "pxr/usd/usdGeom/mesh.h"
#include "pxr/usd/usdLux/lightAPI.h"
#include "pxr/usd/usdShade/shader.h"
#include "pxr/usd/usdShade/material.h"
#include "pxr/base/gf/vec3f.h"
#include "pxr/base/tf/token.h"
#include "pxr/usd/sdf/assetPath.h"

#pragma warning(pop)

#include "Scene/Material.h"

namespace SceneLoaderUSD
{
    namespace MaterialUtils_Mtlx
    {
        inline std::string RecurseTexNodeGraph(const pxr::UsdShadeShader& surface)
        {
            if (!surface)
                return "";

            const auto fileInput = surface.GetInput(pxr::TfToken("file"));
            if (fileInput)
            {
                pxr::SdfAssetPath assetPath;
                if (fileInput.GetAttr().Get(&assetPath))
                {
                    const std::string& resolvedPath = assetPath.GetResolvedPath();
                    return resolvedPath.empty() ? assetPath.GetAssetPath() : resolvedPath;
                }
            }

            for (const auto& input : surface.GetInputs())
            {
                pxr::UsdShadeConnectableAPI source;
                pxr::TfToken name;
                pxr::UsdShadeAttributeType sourceType;

                if (input.GetConnectedSource(&source, &name, &sourceType))
                {
                    const pxr::UsdShadeShader nextSurface(source.GetPrim());
                    return RecurseTexNodeGraph(nextSurface);
                }
            }

            return "";
        }

        inline std::string ExtractTextureFile(const pxr::UsdShadeShader& surface, const char* inputName)
        {
            const auto input = surface.GetInput(pxr::TfToken(inputName));
            if (!input)
                return "";

            pxr::UsdPrim prim;
            pxr::TfToken sourceName;
            {
                pxr::UsdShadeConnectableAPI source;
                pxr::UsdShadeAttributeType sourceType;

                if (!input.GetConnectedSource(&source, &sourceName, &sourceType))
                    return "";

                prim = source.GetPrim();
            }

            pxr::UsdShadeShader tex(prim);
            if (tex)
            {
                const auto fileInput = tex.GetInput(pxr::TfToken("file"));
                if (fileInput)
                {
                    pxr::SdfAssetPath assetPath;
                    if (fileInput.GetAttr().Get(&assetPath))
                    {
                        const std::string resolvedPath = assetPath.GetResolvedPath();
                        return resolvedPath.empty() ? assetPath.GetAssetPath() : resolvedPath;
                    }
                }
            }

            pxr::UsdShadeNodeGraph graph(prim);
            if (graph)
            {
                pxr::UsdShadeOutput output = graph.GetOutput(sourceName);

                pxr::UsdShadeConnectableAPI source;
                pxr::TfToken name;
                pxr::UsdShadeAttributeType sourceType;

                if (output && output.GetConnectedSource(&source, &name, &sourceType))
                {
                    return RecurseTexNodeGraph(source);
                }
            }


            return "";
        }

        inline int TryFetchTexture(const pxr::UsdShadeShader& surface, std::vector<const char*>& textureList, const std::vector<const char*>& inputNames)
        {
            std::string path;

            bool found = false;
            for (auto& inputName : inputNames)
            {
                path = ExtractTextureFile(surface, inputName);
                if (!path.empty())
                {
                    found = true;
                    break;
                }
            }
            if (!found)
                return -1;

            const auto it = std::ranges::find(textureList, path);
            if (it == textureList.end())
            {
                const char* copyPath = _strdup(path.c_str());
                textureList.emplace_back(copyPath);
                return static_cast<int>(textureList.size() - 1);
            }
            return static_cast<int>(it - textureList.begin());
        }
    }

    // TODO: Not extensive enough
    template <typename T>
    bool RecurseMtlXValue(const pxr::UsdShadeInput& input, T& value)
    {
        if (!input)
            return false;

        if (input.Get(&value))
            return true;

        pxr::UsdShadeConnectableAPI source;
        pxr::TfToken sourceName;
        pxr::UsdShadeAttributeType sourceType;

        if (!input.GetConnectedSource(
            &source,
            &sourceName,
            &sourceType))
        {
            return false;
        }

        if (const auto srcInput = source.GetInput(sourceName))
            return RecurseMtlXValue(srcInput, value);
        return false;
    }

    inline Material ExtractMaterial_MaterialX(const pxr::UsdShadeShader& surface, std::vector<const char*>& textureList)
    {
        Material material{};

        material.TexIdxAlbedo               = MaterialUtils_Mtlx::TryFetchTexture(surface, textureList, {"albedo", "diffuse", "diffuse_color", "base_color"});
        material.TexIdxNormal               = MaterialUtils_Mtlx::TryFetchTexture(surface, textureList, {"normal", "geometry_normal"});
        material.TexIdxRoughness            = MaterialUtils_Mtlx::TryFetchTexture(surface, textureList, {"roughness", "specular_roughness"});
        material.TexIdxMetallic             = MaterialUtils_Mtlx::TryFetchTexture(surface, textureList, {"metallic", "metalness"});
        material.TexIdxEmissive             = MaterialUtils_Mtlx::TryFetchTexture(surface, textureList, {"emission_color", "emission"});
        material.TexIdxAnisotropy           = MaterialUtils_Mtlx::TryFetchTexture(surface, textureList, {"anisotropy"});
        material.TexIdxClearcoat            = MaterialUtils_Mtlx::TryFetchTexture(surface, textureList, {"clearcoat"});
        material.TexIdxClearcoatRoughness   = MaterialUtils_Mtlx::TryFetchTexture(surface, textureList, {"clearcoatRoughness"});
        material.TexIdxClearcoatNormal      = MaterialUtils_Mtlx::TryFetchTexture(surface, textureList, {"clearCoatNormal"});
        material.TexIdxSheenColor           = MaterialUtils_Mtlx::TryFetchTexture(surface, textureList, {"sheenColor"});
        material.TexIdxSheenRoughness       = MaterialUtils_Mtlx::TryFetchTexture(surface, textureList, {"sheenRoughness"});
        material.TexIdxTransmissionFactor   = MaterialUtils_Mtlx::TryFetchTexture(surface, textureList, {"transmission"});
        material.TexIdxORM                  = MaterialUtils_Mtlx::TryFetchTexture(surface, textureList, {"orm"});
        material.TexIdxOGM                  = MaterialUtils_Mtlx::TryFetchTexture(surface, textureList, {"ogm", "specular_factor", "specular_color", "specular"});

        auto readFloat = [&](const char* name, float& dest)
        {
            const pxr::UsdShadeInput input = surface.GetInput(pxr::TfToken(name));
            if (input && input.GetAttr().HasAuthoredValue())
            {
                input.GetAttr().Get(&dest);
                return true;
            }
            return RecurseMtlXValue(input, dest);
        };
        auto readFloat3 = [&](const char* name, void* dest)
        {
            const pxr::UsdShadeInput input = surface.GetInput(pxr::TfToken(name));
            pxr::GfVec3f vec3{};
            if (input && input.GetAttr().HasAuthoredValue() && input.GetAttr().Get(&vec3))
            {
                memcpy(dest, &vec3, sizeof(float) * 3);
                return true;
            }
            if (RecurseMtlXValue(input, vec3))
            {
                memcpy(dest, &vec3, sizeof(float) * 3);
                return true;
            }
            return false;
        };
        auto readFloatAssignedTexture = [&](const char* name, const int texIdx, float& dest)
        {
            const bool result = readFloat(name, dest);
            if (!result)
                dest = (texIdx == -1) ? 0.0f : 1.0f;
            return result;
        };

        readFloat3                  ("base_color",              &material.Albedo);
        readFloat3                  ("emission",                &material.EmissiveColor);
        readFloat3                  ("transmission_color",      &material.TransmissionColor);

        readFloatAssignedTexture    ("specular_roughness",      material.TexIdxRoughness,           material.Roughness);
        readFloatAssignedTexture    ("metalness",               material.TexIdxMetallic,            material.Metallic);
        readFloatAssignedTexture    ("emissive_strength",       material.TexIdxEmissive,            material.EmissiveStrength);
        readFloatAssignedTexture    ("transmission",            material.TexIdxTransmissionFactor,  material.TransmissionFactor);

        readFloat                   ("specular_factor",      material.SpecularFactor);
        readFloat                   ("aniso_strength",       material.AnisoStrength);
        readFloat                   ("ior",                  material.IOR_N);

        return material;
    }
}

#endif
