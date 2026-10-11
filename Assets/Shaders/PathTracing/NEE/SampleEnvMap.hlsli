#ifndef H_SAMPLE_ENV_MAP_H
#define H_SAMPLE_ENV_MAP_H

LightSample SampleEnvMap(inout RngInfo rngInfo, out LightEval lightEval)
{
    float xi1 = Rand01(rngInfo);
    float xi2 = Rand01(rngInfo);

    float2 uv_env;
    float pdf_env;
    float3 wi_env;
    SampleEnvMapCdf(xi1, xi2, uv_env, wi_env, pdf_env);

    lightEval.Radiance = gTexEnvMap.Sample(gSamplerLinearClamp, uv_env).rgb;
    lightEval.PDF = pdf_env;

    LightSample lightSample = (LightSample)0;
    lightSample.Direction = wi_env;
    lightSample.Distance = INF;
    lightSample.UV = uv_env;
    lightSample.IsDelta = false;
    return lightSample;
}

LightEval EvaluateEnvMap(LightSample lightSample)
{
    LightEval lightEval;
    lightEval.Radiance = gTexEnvMap.Sample(gSamplerLinearClamp, lightSample.UV).rgb;
    lightEval.PDF = EvaluateEnvMapPdf(lightSample.UV);
    return lightEval;
}

#endif