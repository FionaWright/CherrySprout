#ifndef H_MISS_H
#define H_MISS_H

float3 Miss(inout PathState pathState, float3 origin, float3 direction, uint bounceIdx)
{
    float3 Li = float3(0, 0, 0);

    if (FEATURE_ENABLED(EnvironmentMap))
    {
        float2 uv = EaSphereToSquare(direction);
        float3 Le = gTexEnvMap.Sample(gSampler, uv).rgb;

        if (FEATURE_ENABLED(NEE) && pathState.RaySegmentIdx != 0 && !pathState.LastRayDiracDelta)
        {
            float pdf_env = GetEnvMapPdf(uv);
            float m = PowerHeuristic(pathState.LastBxdfPdf, pdf_env);
            Le *= m;
        }

        Li += Le;
    }

    if (FEATURE_ENABLED(DirectionalLight) && bounceIdx >= 1)
    {
        float sunCos = dot(direction, -normalize(gSettings.DirLightDirection));
        if (FEATURE_ENABLED(DirectionalLightDistant))
            Li += gSettings.DirLightColor * gSettings.DirLightIntensity * saturate(sunCos);
        if (sunCos > gSettings.DirLightCosAngularRadius)
            Li += gSettings.DirLightColor * gSettings.DirLightIntensity / (1 - gSettings.DirLightCosAngularRadius);
    }

    return Li;
}

#endif