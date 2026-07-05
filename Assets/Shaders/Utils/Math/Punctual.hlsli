#ifndef H_PUNCTUAL_H
#define H_PUNCTUAL_H

#include "Utils/Constants.h"

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

#endif