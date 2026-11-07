#ifndef H_SHADOWS_H
#define H_SHADOWS_H

float2 GetPcfOffset(inout RngInfo rngInfo, uint poissonIdx)
{
    poissonIdx = (poissonIdx * Rand01(rngInfo)) % POISSON_DISC_COUNT; // TODO: Blue Noise

    float2 offset = poissonIdx & 1 ? gSettingsShadows.PoissonDisc[poissonIdx/2].xy : gSettingsShadows.PoissonDisc[poissonIdx/2].zw;

    // Rotate
    {
        float angle = Rand01(rngInfo) * 6.2831853;

        float s, c;
        sincos(angle, s, c);

        offset = float2(
            offset.x * c - offset.y * s,
            offset.x * s + offset.y * c
        );
    }

    offset *= gSettingsShadows.PcfRadius;

    return offset;
}

float GetShadowFactorDirectional(inout RngInfo rngInfo, float3 shadowPos[SHADOW_MAP_MAX_CASCADES])
{
    uint cascadeIdx = 0; // TODO

    float3 shadowCoords = shadowPos[cascadeIdx];
    shadowCoords.y = 1 - shadowCoords.y;
    //shadowCoords.x = gSettingsShadows.ShadowWidthPercent * (shadowCoords.x + cascadeIdx);

    float pixelDepth = shadowCoords.z + gSettingsShadows.BiasDepthDistant;

    float shadowFactor = 0.0f;

    for (uint i = 0; i < gSettingsShadows.PcfSampleCount; i++)
    {
        float2 offset = GetPcfOffset(rngInfo, i);

        float2 uv_shadow = shadowCoords.xy + (offset * gSettingsShadows.TexelSize);
        float shadowMapDepth = gShadowMapDir.Sample(gSamplerPointClamp, uv_shadow).r;

        shadowFactor += pixelDepth >= shadowMapDepth;
    }

    shadowFactor /= float(gSettingsShadows.PcfSampleCount);

    return shadowFactor;
}

float GetShadowFactorPoint(inout RngInfo rngInfo, PunctualLight light, ShadowMapPuncInfo smPuncInfo, float3 Ns, float4 worldPos)
{
    float3 normalBias = Ns * gSettingsShadows.NormalBiasPoint;
    float4 biasedPos = float4(worldPos.xyz + normalBias, worldPos.w);

    float3 un_wi = light.Position - biasedPos.xyz;
    float pixelDepth = max3(abs(un_wi.x), abs(un_wi.y), abs(un_wi.z));
    pixelDepth = UnlinearizeDepth(pixelDepth, smPuncInfo.NearPlane, smPuncInfo.FarPlane);
    pixelDepth += gSettingsShadows.BiasDepthPoint;
    pixelDepth = saturate(pixelDepth);

    float3 base_dir = normalize(-un_wi);

    float shadowFactor = 0.0f;

    for (uint i = 0; i < gSettingsShadows.PcfSampleCount; i++)
    {
        ShadingFrame sFrame = CreateShadingFrame(base_dir);

        float2 offset2D = GetPcfOffset(rngInfo, i) / (200.0f);
        float3 offset = (sFrame.T * offset2D.x + sFrame.B * offset2D.y);

        float3 dir = normalize(base_dir + offset);

        float shadowMapDepth = gShadowMapsPoint[smPuncInfo.ShadowMapIndex].Sample(gSamplerPointClamp, dir).r;

        shadowFactor += pixelDepth >= shadowMapDepth;
    }

    shadowFactor /= float(gSettingsShadows.PcfSampleCount);

    return shadowFactor;
}

#endif