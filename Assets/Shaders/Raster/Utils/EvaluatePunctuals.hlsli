#ifndef H_RASTER_EVALUATE_PUNCTUAL_H
#define H_RASTER_EVALUATE_PUNCTUAL_H

#include "Raster/Utils/Shadows.hlsli"

float3 EvaluatePunctuals(inout RngInfo rngInfo, HitInfo hitInfo, float3 wo, float4 worldPos)
{
    uint lightCount, _;
    gPunctuals.GetDimensions(lightCount, _);

    float3 E_direct = 0.0f;

    for (uint i = 0; i < lightCount; i++)
    {
        PunctualLight light = gPunctuals[i];

        LightEval lightEval;
        LightSample lightSample = EvaluatePunctualLight(light, hitInfo.HitPos, lightEval);

        float3 wi = lightEval.Direction;
        float NdL = max(1e-6f, dot(hitInfo.Ns_ff, wi));
        if (NdL > 0.0f && any(lightEval.Radiance > 0))
        {
            ShadowMapPuncInfo smPuncInfo = gPunctualShadowMapInfo[i];

            float shadowFactor = 1.0f;
            if (ForwardFlagGet(gSettings.Flags, ForwardFlags::eShadowMaps))
            {
                if (light.Type == PunctualLightType::eDistant)
                {
                    continue; // TODO
                }
                else
                {
                    shadowFactor = GetShadowFactorPoint(rngInfo, light, smPuncInfo, hitInfo.Ns_ff, worldPos);
                }
            }

            float3 un_wi = light.Position - worldPos.xyz;
            float3 dir = normalize(-un_wi);
            float shadowMapDepth = gShadowMapsPoint[smPuncInfo.ShadowMapIndex].Sample(gSamplerPointClamp, dir).r;

            //return float3(dir);
            //return float3(shadowFactor.xxx);
            return float3(shadowMapDepth.xxx);

            BxdfEval bxdfEval = BxDF::Evaluate(hitInfo, wo, wi, LobeTag::eAll);

            //float radMult = 100.0f;

            if (bxdfEval.PDF > 0.0f)
                E_direct += shadowFactor * NdL * lightEval.Radiance * bxdfEval.Radiance;
        }
    }

    return E_direct;
}

#endif