#include "Utils/HlslUtils.hlsli"

Texture2D<float4> gSource : register(t0);
RWTexture2D<float4> gDest : register(u0);

[numthreads(16, 16, 1)]
void CSMain(uint3 DTid : SV_DispatchThreadID)
{
    uint2 dimDest;
    gDest.GetDimensions(dimDest.x, dimDest.y);

    uint2 pixelCoordDest = DTid.xy;

    if (pixelCoordDest.x >= dimDest.x || pixelCoordDest.y >= dimDest.y)
        return;

    uint2 pixelCoordSource = pixelCoordDest * 2;

#define NUM_SAMPLES 4

    uint2 offsets[NUM_SAMPLES] = {
        uint2(0,0),
        uint2(0,1),
        uint2(1,0),
        uint2(1,1),
    };

    float4 rollingColor = 0.0f;
    [unroll]
    for (int i = 0; i < NUM_SAMPLES; i++)
    {
        float2 coords = pixelCoordSource + offsets[i];
        float4 sample = gSource[coords];
        sample.rgb = SRGB_to_LRGB_Fast(sample.rgb);
        rollingColor += sample;
    }

    rollingColor /= float(NUM_SAMPLES);

    rollingColor.rgb = LRGB_to_SRGB_Fast(rollingColor.rgb);

    gDest[pixelCoordDest] = rollingColor;
}