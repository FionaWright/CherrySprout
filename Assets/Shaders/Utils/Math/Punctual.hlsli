#ifndef H_PUNCTUAL_H
#define H_PUNCTUAL_H

#include "Utils/Constants.h"
#include "PathTracing/Debug/Assert.hlsli"
#include "PathTracing/Debug/Scales.hlsli"
#include "PathTracing/Structs.h"

// light.Intensity is Candela (lm/sr)
float3 PointLightWeight(PunctualLight light)
{
    float3 candela = light.Color * light.Intensity;
    float3 radiantIntensity = candela * CANDELA_TO_RADIANT_INTENSITY;
    return radiantIntensity;
}

// light.Intensity is Illuminance (lm/m^2) ~= Luminous Efficacy * Irradiance (W/m^2)
float3 DistantLightWeight(PunctualLight light)
{
    // Assuming distant day-light spectrum
    // Assuming light approximately subtended as sun is from earth
    float3 lux = light.Color * light.Intensity;
    float3 irradiance = ILLUMINANCE_TO_IRRADIANCE * lux;
    float3 radiantIntensity = irradiance * SUN_SUBTENDED_SPHERE_COS_ANGULAR_RADIUS * INV_SUN_SUBTENDED_SPHERE_SOLID_ANGLE;

    // radiantIntensity is radiometrically correct but results in over-sampling
    // Use log scale to compress estimate into useful range
    float3 relImportance = log(1.0f + 0.001f * radiantIntensity);
    return relImportance;
}

// light.Intensity is Candela (lm/sr)
float3 SpotLightWeight(PunctualLight light)
{
    float3 candela = light.Color * light.Intensity;
    float solidAngleRatio = (1.0f - cos(light.SpotOuterAngle));
    float3 radiantIntensity = candela * solidAngleRatio * CANDELA_TO_RADIANT_INTENSITY;
    return radiantIntensity;
}

// ?
// light.Intensity is Illuminance (lm/m^2) ~= Luminous Efficacy * Irradiance (W/m^2)
float3 RectLightWeight(PunctualLight light)
{
    float3 lux = light.Color * light.Intensity;
    float3 irradiance = ILLUMINANCE_TO_IRRADIANCE * lux;

    float area = light.RectWidth * light.RectHeight;

    float3 lum = irradiance * area;
    return lum;
}

float PunctualLightWeight(PunctualLight light)
{
    float3 weight;

    switch (light.Type)
    {
    case PunctualLightType::ePoint:
        weight = light.Intensity * Luminance(light.Color);
        break;

    case PunctualLightType::eDistant:
        weight = light.Intensity * Luminance(light.Color);
        break;

    case PunctualLightType::eSpot:
        weight = SpotLightWeight(light);
        break;

    case PunctualLightType::eRect:
        weight = RectLightWeight(light);
        break;

    default:
        weight = NAN;
    }

    return length(weight);
}

LightSample EvaluatePointLight(PunctualLight light, float3 position, out LightEval lightEval)
{
    LightSample lightSample = (LightSample)0;

    float3 L = light.Position - position;

    float distance2 = dot(L, L);

    if (distance2 <= 1e-8f)
        return lightSample;

    float radius = max(light.PointRadius, 1e-4f);
    DBG_SCALE_POINT_LIGHT_RADIUS(radius);

    float radius2 = radius * radius;

    float distance = sqrt(distance2);

    lightEval.Direction = normalize(L);
    lightEval.Distance = max(distance - radius, 0.0f);

    float attenuation = 1.0f / max(distance2, radius2);

    lightEval.Radiance = light.Color * (light.Intensity * attenuation);
    DBG_SCALE_INTENSITY_POINT(lightEval.Radiance);

    return lightSample;
}

LightSample EvaluateDistantLight(PunctualLight light, out LightEval lightEval)
{
    LightSample lightSample = (LightSample)0;

    lightEval.Direction = normalize(light.Position);
    lightEval.Distance = INF;

    lightEval.Radiance = light.Color * light.Intensity;
    DBG_SCALE_INTENSITY_DISTANT(lightEval.Radiance);

    return lightSample;
}

LightSample EvaluateSpotLight(PunctualLight light, float3 position, out LightEval lightEval)
{
    LightSample lightSample = (LightSample)0;

    float3 L = light.Position - position;
    float distance2 = dot(L, L);

    if (distance2 <= 1e-8f)
        return lightSample;

    float distance = sqrt(distance2);

    lightEval.Direction = L / distance;
    lightEval.Distance = distance;

    float3 lightToPoint = -lightEval.Direction;

    float cosTheta = dot(normalize(light.SpotOrRectDirection), lightToPoint);

    float cosInner = cos(light.SpotInnerAngle);
    float cosOuter = cos(light.SpotOuterAngle);

    float spotFactor = saturate((cosTheta - cosOuter) /
                                (cosInner - cosOuter));

    spotFactor *= spotFactor * (3.0f - 2.0f * spotFactor);

    float attenuation = spotFactor / distance2;

    lightEval.Radiance = light.Color * (light.Intensity * attenuation);
    DBG_SCALE_INTENSITY_SPOT(lightEval.Radiance);

    return lightSample;
}

// TODO: Should sample a point on the rect light
// TODO: Rect area not taking into account scene scale
LightSample EvaluateRectLight(PunctualLight light, float3 position, out LightEval lightEval)
{
    LightSample lightSample = (LightSample)0;

    float3 L = light.Position - position;
    float distance2 = dot(L, L);

    if (distance2 <= 1e-8f)
        return lightSample;

    float distance = sqrt(distance2);

    lightEval.Direction = L / distance;
    lightEval.Distance = distance;

    float cosTheta = dot(normalize(light.SpotOrRectDirection), -L);

    lightEval.Radiance = light.Color * light.Intensity * cosTheta / distance2;
    DBG_SCALE_INTENSITY_RECT(lightEval.Radiance);

    return lightSample;
}

// TODO: Refactor into Sample and Evaluate

LightSample EvaluatePunctualLight(PunctualLight light, float3 position, out LightEval lightEval)
{
    LightSample lightSample = (LightSample)0;

    switch (light.Type)
    {
    case PunctualLightType::ePoint:
        lightSample = EvaluatePointLight(light, position, lightEval);
        break;

    case PunctualLightType::eDistant:
        lightSample = EvaluateDistantLight(light, lightEval);
        break;

    case PunctualLightType::eSpot:
        lightSample = EvaluateSpotLight(light, position, lightEval);
        break;

    case PunctualLightType::eRect:
        lightSample = EvaluateRectLight(light, position, lightEval);
        break;

    default:
        DBG_ASSERT_FAIL(UNSUPPORTED_LIGHT_TYPE);
        break;
    }

    lightEval.PDF = 1.0f;
    lightEval.IsDelta = true;
    return lightSample;
}

#endif