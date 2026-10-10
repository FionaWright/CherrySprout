#ifndef H_MISS_H
#define H_MISS_H

#include "PathTracing/MIS.hlsli"
#include "PathTracing/NEE/LSD/SampleEnvMapCdf.hlsli"
#include "PathTracing/NEE/LSD/SampleLSD.hlsli"
#include "PathTracing/Debug/Scales.hlsli"

float3 Miss(inout PathState pathState, uint bounceIdx)
{
    if (FEAT_DBG(FurnaceTestHHE))
        return 0.0f;
    if (FEAT_DBG(FurnaceTestHDR))
        return 1.0f;

    float3 Li = float3(0, 0, 0);

    if (FEAT_CORE(EnvironmentMap))
    {
        float2 uv = EaSphereToSquare(pathState.Desc.Direction);
        float3 Le = gTexEnvMap.Sample(gSamplerLinearWrap, uv).rgb;

        DBG_SCALE_INTENSITY_ENV_MAP(Le);

        bool envMapInLSD = gSettings.LsdBasePunctuals >= 1;
        if (FEAT_CORE(NEE) && envMapInLSD && !pathState.IsPrimaryRay && !pathState.LastRayWasDiracDelta)
        {
            float pdf_lsd;
            EvaluateLSD(0, pdf_lsd);

            float pdf_env = pdf_lsd * EvaluateEnvMapPdf(uv);
            float m = PowerHeuristic(pathState.LastBxdfPdf, pdf_env, 1, gSettings.DirectNumSamples);
            Le *= m;
        }

        DBG_OUTPUT2(uv, EnvironmentMapUV);
        DBG_OUTPUT3(Le, EnvironmentMap);

        Li += Le;
    }

    if (FEAT_CORE(DirectionalLight) && bounceIdx >= 1)
    {
        float sunCos = dot(pathState.Desc.Direction, -normalize(gSettings.DirLightDirection));
        if (FEAT_CORE(DirectionalLightDistant))
            Li += gSettings.DirLightColor * gSettings.DirLightIntensity * saturate(sunCos);
        if (sunCos > gSettings.DirLightCosAngularRadius)
            Li += gSettings.DirLightColor * gSettings.DirLightIntensity / (1 - gSettings.DirLightCosAngularRadius);
    }

    DBG_SCALE_INTENSITY_INDIRECT(Li);

    return Li;
}

#endif