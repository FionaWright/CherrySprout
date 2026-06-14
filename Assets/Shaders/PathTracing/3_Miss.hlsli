#ifndef H_MISS_H
#define H_MISS_H

float3 Miss(float3 origin, float3 direction, uint bounceIdx)
{
    float3 Li = float3(0, 0, 0);

    if (FEATURE_ENABLED(EnvironmentMap))
    {
        float2 uv = EaSphereToSquare(direction);
        Li += saturate(gTexEnvMap.Sample(gSampler, uv).rgb);
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