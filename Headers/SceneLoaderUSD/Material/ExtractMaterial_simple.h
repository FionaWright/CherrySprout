#ifndef H_EXTRACT_MATERIAL_SIMPLE_H
#define H_EXTRACT_MATERIAL_SIMPLE_H

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
    namespace MaterialUtils_Simple
    {
        inline std::string ExtractTextureFile(const pxr::UsdShadeShader& surface, const char* inputName)
        {
            const auto input = surface.GetInput(pxr::TfToken(inputName));
            if (!input || !input.HasConnectedSource())
                return "";

            for (auto& attr : input.GetValueProducingAttributes())
            {
                const pxr::UsdShadeShader tex(attr.GetPrim());
                if (!tex)
                    continue;

                const auto fileInput = tex.GetInput(pxr::TfToken("file"));
                if (!fileInput)
                    continue;

                pxr::SdfAssetPath assetPath;
                if (fileInput.GetAttr().Get(&assetPath))
                {
                    const std::string resolvedPath = assetPath.GetResolvedPath();
                    return resolvedPath.empty() ? assetPath.GetAssetPath() : resolvedPath;
                }
            }
            return "";
        }

        inline int TryFetchTexture(const pxr::UsdShadeShader& surface, std::vector<const char*>& textureList, const char* inputName)
        {
            const std::string path = ExtractTextureFile(surface, inputName);
            if (path.empty())
                return -1;

            const auto it = std::ranges::find(textureList, path);
            if (it == textureList.end())
            {
                const char* copyPath = _strdup(path.c_str());
                textureList.emplace_back(copyPath);
                return textureList.size() - 1;
            }
            return it - textureList.begin();
        }
    }

    inline Material ExtractMaterial_Simple(const pxr::UsdShadeShader& surface, std::vector<const char*>& textureList)
    {
        Material material{};

        material.TexIdxAlbedo               = MaterialUtils_Simple::TryFetchTexture(surface, textureList, "diffuseColor");
        material.TexIdxNormal               = MaterialUtils_Simple::TryFetchTexture(surface, textureList, "normal");
        material.TexIdxRoughness            = MaterialUtils_Simple::TryFetchTexture(surface, textureList, "roughness");
        material.TexIdxMetallic             = MaterialUtils_Simple::TryFetchTexture(surface, textureList, "metallic");
        material.TexIdxEmissiveStrength     = MaterialUtils_Simple::TryFetchTexture(surface, textureList, "emissiveColor");
        material.TexIdxAnisotropy           = MaterialUtils_Simple::TryFetchTexture(surface, textureList, "anisotropy");
        material.TexIdxClearcoat            = MaterialUtils_Simple::TryFetchTexture(surface, textureList, "clearcoat");
        material.TexIdxClearcoatRoughness   = MaterialUtils_Simple::TryFetchTexture(surface, textureList, "clearcoatRoughness");
        material.TexIdxClearcoatNormal      = MaterialUtils_Simple::TryFetchTexture(surface, textureList, "clearCoatNormal");
        material.TexIdxSheenColor           = MaterialUtils_Simple::TryFetchTexture(surface, textureList, "sheenColor");
        material.TexIdxSheenRoughness       = MaterialUtils_Simple::TryFetchTexture(surface, textureList, "sheenRoughness");
        material.TexIdxTransmissionFactor   = MaterialUtils_Simple::TryFetchTexture(surface, textureList, "transmission");

        auto readFloat = [&](const char* name, float& dest)
        {
            const pxr::UsdShadeInput input = surface.GetInput(pxr::TfToken(name));
            if (input)
            {
                input.GetAttr().Get(&dest);
                return true;
            }
            return false;
        };
        auto readFloat3 = [&](const char* name, void* dest)
        {
            const pxr::UsdShadeInput input = surface.GetInput(pxr::TfToken(name));
            pxr::GfVec3f vec3{};
            if (input && input.GetAttr().Get(&vec3))
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

        readFloat3                  ("diffuseColor",        &material.BaseColor);
        readFloat3                  ("emissiveColor",       &material.EmissiveColor);
        readFloat3                  ("transmissionColor",   &material.TransmissionColor);

        readFloatAssignedTexture    ("roughness",           material.TexIdxRoughness,           material.Roughness);
        readFloatAssignedTexture    ("metallic",            material.TexIdxMetallic,            material.Metallic);
        readFloatAssignedTexture    ("emissiveStrength",    material.TexIdxEmissiveStrength,    material.EmissiveStrength);
        readFloatAssignedTexture    ("transmission",        material.TexIdxTransmissionFactor,  material.TransmissionFactor);

        readFloat                   ("specularFactor",  material.SpecularFactor);
        readFloat                   ("anisoStrength",   material.AnisoStrength);
        readFloat                   ("ior",             material.IOR_N);

        return material;
    }
}

#endif