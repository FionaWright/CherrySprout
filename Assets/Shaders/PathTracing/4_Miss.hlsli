#ifndef H_MISS_H
#define H_MISS_H

#include "PathTracing/MIS.hlsli"
#include "PathTracing/NEE/LSD/SampleEnvMapCdf.hlsli"
#include "PathTracing/Debug/Scales.hlsli"

float3 Miss(inout PathState pathState, uint bounceIdx)
{
    if (DEBUG_ENABLED(FurnaceTestHHE))
        return 0.0f;
    if (DEBUG_ENABLED(FurnaceTestHDR))
        return 1.0f;

    float3 Li = float3(0, 0, 0);

    if (FEATURE_ENABLED(EnvironmentMap))
    {
        float2 uv = EaSphereToSquare(pathState.Desc.Direction);
        float3 Le = gTexEnvMap.Sample(gSamplerLinearWrap, uv).rgb;

        DBG_SCALE_INTENSITY_ENV_MAP(Le);

        if (FEATURE_ENABLED(NEE) && !pathState.IsPrimaryRay && !pathState.LastRayWasDiracDelta)
        {
            float pdf_env = EvaluateEnvMapPdf(uv);
            float m = PowerHeuristic(pathState.LastBxdfPdf, pdf_env, 1, gSettings.DirectNumSamples);
            Le *= m;
        }

        DBG_OUTPUT2(uv, EnvironmentMapUV);
        DBG_OUTPUT3(Le, EnvironmentMap);

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

    DBG_SCALE_INTENSITY_INDIRECT(Li);

    return Li;
}

#endif