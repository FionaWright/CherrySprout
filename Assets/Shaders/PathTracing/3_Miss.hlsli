#ifndef H_MISS_H
#define H_MISS_H

float3 Miss(float3 origin, float3 direction, uint bounceIdx)
{
    //return float3(1, 1, 1);
    //if (bounceIdx == 0) // TODO
        //return abs(direction);

    float3 Li = float3(0, 0, 0);

    if (FEATURE_ENABLED(EnvironmentMap))
    {
        float2 uv;
        if (FEATURE_ENABLED(EnvironmentMapEA))
            uv = EaSphereToSquare(direction);
        else
            uv = PanoSphereToSquare(direction);

        Li += saturate(gTexEnvMap.Sample(gSampler, uv).rgb);
    }

    if (false && bounceIdx >= 1)
    {
        float sunCos = dot(direction, -normalize(gSettings.DirLightDirection));
        //if (cDirLightIsDistant)
            Li += gSettings.DirLightColor * gSettings.DirLightIntensity * saturate(sunCos);
        //if (sunCos > gSettings.DirLightCosAngularRadius)
        //    Li += gSettings.DirLightColor * gSettings.DirLightIntensity;
    }

    return Li;
}

#endif