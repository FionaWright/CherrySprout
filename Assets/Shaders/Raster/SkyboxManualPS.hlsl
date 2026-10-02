#include "Utils/SharedUtils.h"

struct VsOut
{
    float4 Position : SV_Position;
    float3 ViewDirection : TEXCOORD0;
};

Texture2DArray<float4> gCubemap : register(t0);
SamplerState gSampler : register(s0);

float4 PSMain(VsOut input) : SV_Target
{
    uint sourceFace;
    float2 sourceUV;
    CubemapSphereToCube(input.ViewDirection, sourceFace, sourceUV);

    sourceUV = sourceUV * 0.5 + 0.5;

    float3 col = gCubemap.SampleLevel(gSampler, float3(sourceUV, sourceFace), 0).rgb;
    return float4(col, 1);
}