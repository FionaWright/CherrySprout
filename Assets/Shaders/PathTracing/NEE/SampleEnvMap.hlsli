#ifndef H_SAMPLE_ENV_MAP_H
#define H_SAMPLE_ENV_MAP_H

LightSample SampleEnvMap(inout RngInfo rngInfo)
{
    float xi1 = Rand01(rngInfo);
    float xi2 = Rand01(rngInfo);

    float2 uv_env;
    float pdf_env;
    float3 wi_env;
    SampleEnvMapCdf(xi1, xi2, uv_env, wi_env, pdf_env);

    LightSample lightSample = (LightSample)0;

    lightSample.Direction = wi_env;
    lightSample.Distance = INF;

    lightSample.PDF = pdf_env;

    lightSample.Radiance = gTexEnvMap.Sample(gSamplerLinearWrap, uv_env).rgb;

    return lightSample;
}

#endif