#ifndef H_SAMPLE_ENV_MAP_H
#define H_SAMPLE_ENV_MAP_H

LightSample SampleEnvMap(float2 rng, out LightEval lightEval)
{
    float2 uv_env;
    float pdf_env;
    float3 wi_env;
    SampleEnvMapCdf(rng.x, rng.y, uv_env, wi_env, pdf_env);

    lightEval.Radiance = gTexEnvMap.Sample(gSamplerLinearClamp, uv_env).rgb;
    lightEval.PDF = pdf_env;
    lightEval.Direction = wi_env;
    lightEval.Distance = INF;
    lightEval.IsDelta = false;

    LightSample lightSample = (LightSample)0;
    lightSample.BaryOrUV.xy = uv_env;
    return lightSample;
}

LightEval EvaluateEnvMap(LightSample lightSample)
{
    LightEval lightEval;
    lightEval.Radiance = gTexEnvMap.Sample(gSamplerLinearClamp, lightSample.BaryOrUV.xy).rgb;
    lightEval.PDF = EvaluateEnvMapPdf(lightSample.BaryOrUV.xy);
    lightEval.Direction = EaSquareToSphere(lightSample.BaryOrUV.xy);
    lightEval.Distance = INF;
    lightEval.IsDelta = false;
    return lightEval;
}

#endif