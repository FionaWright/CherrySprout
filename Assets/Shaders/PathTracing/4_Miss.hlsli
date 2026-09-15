#ifndef H_MISS_H
#define H_MISS_H

#include "PathTracing/MIS.hlsli"
#include "PathTracing/NEE/SampleEnvMapCdf.hlsli"
#include "PathTracing/Debug/Scales.hlsli"

float3 Miss(inout PathState pathState, uint bounceIdx)
{
    float3 Li = float3(0, 0, 0);

    if (FEATURE_ENABLED(EnvironmentMap))
    {
        float2 uv = EaSphereToSquare(pathState.Desc.Direction);
        float3 Le = gTexEnvMap.Sample(gSampler, uv).rgb;
        DBG_SCALE_INTENSITY_ENV_MAP(Le);

        if (FEATURE_ENABLED(NEE) && pathState.RaySegmentIdx != 0 && !pathState.LastRayWasDiracDelta)
        {
            float pdf_env = GetEnvMapPdf(uv);
            float m = PowerHeuristic(pathState.LastBxdfPdf, pdf_env, 1, gSettings.DirectNumSamples);
            Le *= m;
        }

        Li += Le;
    }

    if (FEATURE_ENABLED(DirectionalLight) && bounceIdx >= 1)
    {
        float sunCos = dot(pathState.Desc.Direction, -normalize(gSettings.DirLightDirection));
        if (FEATURE_ENABLED(DirectionalLightDistant))
            Li += gSettings.DirLightColor * gSettings.DirLightIntensity * saturate(sunCos);
        if (sunCos > gSettings.DirLightCosAngularRadius)
            Li += gSettings.DirLightColor * gSettings.DirLightIntensity / (1 - gSettings.DirLightCosAngularRadius);
    }

    return Li;
}

#endif