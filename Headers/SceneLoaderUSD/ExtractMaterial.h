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

#include "Importer.h"
#include "Scene/Material.h"

namespace SceneLoaderUSD
{
    Material ExtractMaterial_Simple(const pxr::UsdShadeShader& surface, const pxr::UsdShadeMaterial& mat);
    Material ExtractMaterial_MaterialX(const pxr::UsdShadeShader& surface, const pxr::UsdShadeMaterial& mat);

    inline Material ExtractMaterial(const pxr::UsdShadeMaterial& mat)
    {
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
                    pxr::UsdShadeShader surface = pxr::UsdShadeShader(source.GetPrim());
                    return ExtractMaterial_Simple(surface, mat);
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
                    pxr::UsdShadeShader surface = pxr::UsdShadeShader(source.GetPrim());
                    return ExtractMaterial_Simple(surface, mat);
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
                    pxr::UsdShadeShader surface = pxr::UsdShadeShader(source.GetPrim());
                    return ExtractMaterial_MaterialX(surface, mat);
                }
            }
        }

        throw std::runtime_error("Unsupported material type");
    }

    inline Material ExtractMaterial_Simple(const pxr::UsdShadeShader& surface, const pxr::UsdShadeMaterial& mat)
    {
        auto readFloat = [&](const char* name, float& dest)
        {
            const pxr::UsdShadeInput input = surface.GetInput(pxr::TfToken(name));
            if (input)
                input.GetAttr().Get(&dest);
        };
        auto readFloat3 = [&](const char* name, hlsl::float3& dest)
        {
            const pxr::UsdShadeInput input = surface.GetInput(pxr::TfToken(name));
            pxr::GfVec3f vec3{};
            if (input && input.GetAttr().Get(&vec3))
                memcpy(&dest, &vec3, sizeof(float) * 3);
        };
        auto readFloat3to4 = [&](const char* name, hlsl::float4& dest)
        {
            const pxr::UsdShadeInput input = surface.GetInput(pxr::TfToken(name));
            pxr::GfVec3f vec3{};
            if (input && input.GetAttr().Get(&vec3))
                memcpy(&dest, &vec3, sizeof(float) * 3);
        };

        // TODO: Textures

        Material material{};

        readFloat3to4("diffuseColor", material.BaseColor);
        readFloat3("emissiveColor", material.EmissiveColor);
        readFloat("roughness", material.Roughness);
        readFloat("metallic", material.Metallic);

        return material;
    }

    template<typename T>
    void RecurseMtlXValue(const pxr::UsdShadeInput& input, T& value)
    {
        if (!input)
            return;

        if (input.Get(&value))
            return;

        pxr::UsdShadeConnectableAPI source;
        pxr::TfToken sourceName;
        pxr::UsdShadeAttributeType sourceType;

        if (!input.GetConnectedSource(
                &source,
                &sourceName,
                &sourceType))
        {
            return;
        }

        if (const auto srcInput = source.GetInput(sourceName))
            RecurseMtlXValue(srcInput, value);
    }

    inline Material ExtractMaterial_MaterialX(const pxr::UsdShadeShader& surface, const pxr::UsdShadeMaterial& mat)
    {
        auto readFloat = [&](const char* name, float& dest)
        {
            const pxr::UsdShadeInput input = surface.GetInput(pxr::TfToken(name));
            if (input && input.GetAttr().HasAuthoredValue())
            {
                input.GetAttr().Get(&dest);
                return;
            }
            RecurseMtlXValue(input, dest);
        };
        auto readFloat3 = [&](const char* name, hlsl::float3& dest)
        {
            const pxr::UsdShadeInput input = surface.GetInput(pxr::TfToken(name));
            pxr::GfVec3f vec3{};
            if (input && input.GetAttr().HasAuthoredValue() && input.GetAttr().Get(&vec3))
            {
                memcpy(&dest, &vec3, sizeof(float) * 3);
                return;
            }
            RecurseMtlXValue(input, vec3);
            memcpy(&dest, &vec3, sizeof(float) * 3);

        };
        auto readFloat3to4 = [&](const char* name, hlsl::float4& dest)
        {
            const pxr::UsdShadeInput input = surface.GetInput(pxr::TfToken(name));
            pxr::GfVec3f vec3{};
            if (input && input.GetAttr().Get(&vec3))
            {
                memcpy(&dest, &vec3, sizeof(float) * 3);
                return;
            }
            RecurseMtlXValue(input, vec3);
            memcpy(&dest, &vec3, sizeof(float) * 3);
        };

        // TODO: Textures

        Material material{};

        readFloat3to4("diffuseColor", material.BaseColor);
        readFloat3("emissiveColor", material.EmissiveColor);
        readFloat("roughness", material.Roughness);
        readFloat("metallic", material.Metallic);

        return material;
    }
}

#endif