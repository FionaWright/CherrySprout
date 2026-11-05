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

float GetShadowFactor(inout RngInfo rngInfo, float3 shadowPos[SHADOW_MAP_MAX_CASCADES])
{
    uint cascadeIdx = 0; // TODO

    float3 shadowCoords = shadowPos[cascadeIdx];
    shadowCoords.y = 1 - shadowCoords.y;
    //shadowCoords.x = gSettingsShadows.ShadowWidthPercent * (shadowCoords.x + cascadeIdx);

    float pixelDepth = shadowCoords.z + gSettingsShadows.BiasDepth;

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

#endif