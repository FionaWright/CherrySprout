#ifndef H_EXTRACT_LIGHT_H
#define H_EXTRACT_LIGHT_H

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
#include <pxr/usd/usdLux/sphereLight.h>
#include <pxr/usd/usdLux/diskLight.h>
#include <pxr/usd/usdLux/rectLight.h>
#include <pxr/usd/usdLux/cylinderLight.h>
#include <pxr/usd/usdLux/domeLight.h>
#include <pxr/usd/usdLux/shapingAPI.h>
#include <pxr/usd/usdGeom/xformCache.h>
#include <pxr/base/gf/matrix4d.h>
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
#include "SceneLoaderUSD.h"

namespace SceneLoaderUSD
{
    inline PunctualLight ExtractLight(const pxr::UsdPrim& prim, pxr::UsdGeomXformCache& xformCache)
    {
        PunctualLight light{};

        // World-space transform
        const pxr::GfMatrix4d world = xformCache.GetLocalToWorldTransform(prim);
        pxr::GfVec3d translation = world.ExtractTranslation();
        light.Position = hlsl::float3(
            static_cast<float>(translation[0]),
            static_cast<float>(translation[1]),
            static_cast<float>(translation[2]));

        // Common light properties
        const pxr::UsdLuxLightAPI usdLight(prim);

        light.Intensity = 1.0f;
        usdLight.GetIntensityAttr().Get(&light.Intensity);

        pxr::GfVec3f color(1.0f);
        usdLight.GetColorAttr().Get(&color);
        light.Color = hlsl::float3(color[0], color[1], color[2]);

        // Defaults
        light.PointRadius = 0.0f;
        light.SpotInnerAngle = 0.0f;
        light.SpotOuterAngle = 0.0f;

        // TODO: Disk, Cylinder, Dome

        // Determine light type
        if (prim.IsA<pxr::UsdLuxSphereLight>())
        {
            light.Type = PunctualLightType::ePoint;
            pxr::UsdLuxSphereLight(prim).GetRadiusAttr().Get(&light.PointRadius);
        }
        else if (prim.IsA<pxr::UsdLuxRectLight>())
        {
            light.Type = PunctualLightType::eRect;
            pxr::UsdLuxRectLight rectLight(prim);
            rectLight.GetWidthAttr().Get(&light.RectWidth);
            rectLight.GetHeightAttr().Get(&light.RectHeight);
        }
        else if (prim.IsA<pxr::UsdLuxDistantLight>())
        {
            light.Type = PunctualLightType::eDistant;
        }
        else
        {
            // Fallback
            light.Type = PunctualLightType::ePoint;
        }

        const pxr::UsdLuxShapingAPI shapingAPI(prim);
        const bool isSpot = shapingAPI && !prim.IsA<pxr::UsdLuxDistantLight>() && !prim.IsA<pxr::UsdLuxDomeLight>();
        if (isSpot)
        {
            light.SpotOuterAngle = 0.0f;
            shapingAPI.GetShapingConeAngleAttr().Get(&light.SpotOuterAngle);

            if (light.SpotOuterAngle > 0.0f && light.SpotOuterAngle < 180.0f)
            {
                light.Type = PunctualLightType::eSpot;

                float softness = 0.0f;
                shapingAPI.GetShapingConeSoftnessAttr().Get(&softness);
                light.SpotInnerAngle = light.SpotOuterAngle * (1.0f - softness);

                constexpr float DegToRad = 3.14159265358979323846f / 180.0f;
                light.SpotOuterAngle *= DegToRad;
                light.SpotInnerAngle *= DegToRad;

                pxr::GfVec3d worldDir = world.ExtractRotationMatrix() * pxr::GfVec3d(0, 0, -1); // TODO: Assuming Z-up?
                worldDir.Normalize();
                light.SpotDirection = { static_cast<float>(worldDir[0]), static_cast<float>(worldDir[1]), static_cast<float>(worldDir[2]) };
            }
        }

        return light;
    }
}

#endif