#ifndef H_MISS_H
#define H_MISS_H

float3 Miss(float3 origin, float3 direction, uint bounceIdx)
{
    if (bounceIdx == 0) // TODO
        return abs(direction);

    float3 Li = float3(0, 0, 0);

    //if (cEnvMapEnabled)
    //{
    //    float2 uv;
    //    if (cEnvMapIsEqualArea)
    //        uv = EaSphereToSquare(direction);
    //    else
    //        uv = PanoSphereToSquare(direction);
//
    //    Li += saturate(gEnvMap.Sample(gSampler, uv).rgb);
    //}

    if (true && bounceIdx >= 1)
    {
        float sunCos = dot(direction, -normalize(gSettings.DirLight));
        //if (cDirLightIsDistant)
        //    Li += gSettings.DirLightColor * gSettings.DirLightIntensity * saturate(sunCos);
        if (sunCos > gSettings.DirLightCosAngularRadius)
            Li += gSettings.DirLightColor * gSettings.DirLightIntensity;
    }

    return Li;
}

#endif