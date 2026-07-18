#ifndef H_PUNCTUAL_H
#define H_PUNCTUAL_H

#include "Utils/Constants.h"
#include "PathTracing/Debug/Assert.hlsli"
#include "PathTracing/Debug/Scales.hlsli"

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

    default:
        weight = NAN;
    }

    return length(weight);
}

struct LightSample
{
    float3 Direction;
    float Distance;
    float3 Radiance;
};

LightSample EvaluatePointLight(PunctualLight light, float3 position)
{
    LightSample sample;

    float3 L = light.Position - position;

    float distance2 = dot(L, L);

    if (distance2 <= 1e-8f)
        return sample;

    float radius = max(light.PointRadius, 1e-4f);
    DBG_SCALE_POINT_LIGHT_RADIUS(radius);

    float radius2 = radius * radius;

    float distance = sqrt(distance2);

    sample.Direction = L / distance;
    sample.Distance = max(distance - radius, 0.0f);

    float attenuation = 1.0f / max(distance2, radius2);

    sample.Radiance = light.Color * (light.Intensity * attenuation);

    return sample;
}

LightSample EvaluateDistantLight(PunctualLight light)
{
    LightSample sample;

    sample.Direction = normalize(-light.Position);
    sample.Distance = INF;

    sample.Radiance = light.Color * light.Intensity;

    return sample;
}

LightSample EvaluateSpotLight(PunctualLight light, float3 position)
{
    LightSample sample;

    float3 L = light.Position - position;
    float distance2 = dot(L, L);

    if (distance2 <= 1e-8f)
        return sample;

    float distance = sqrt(distance2);

    sample.Direction = L / distance;
    sample.Distance = distance;

    float3 lightToPoint = -sample.Direction;

    float cosTheta = dot(normalize(light.SpotDirection), lightToPoint);

    float cosInner = cos(light.SpotInnerAngle);
    float cosOuter = cos(light.SpotOuterAngle);

    float spotFactor = saturate((cosTheta - cosOuter) /
                                (cosInner - cosOuter));

    spotFactor *= spotFactor * (3.0f - 2.0f * spotFactor);

    float attenuation = spotFactor / distance2;

    sample.Radiance = light.Color * (light.Intensity * attenuation);

    return sample;
}

LightSample EvaluateLight(PunctualLight light, float3 position)
{
    switch (light.Type)
    {
    case PunctualLightType::ePoint:
        return EvaluatePointLight(light, position);

    case PunctualLightType::eDistant:
        return EvaluateDistantLight(light);

    case PunctualLightType::eSpot:
        return EvaluateSpotLight(light, position);

    default:
        DBG_ASSERT_FAIL(UNSUPPORTED_LIGHT_TYPE);
        LightSample sample;
        sample.Direction = NAN;
        sample.Distance = NAN;
        sample.Radiance = NAN;
        return sample;
    }
}

#endif