#include "Compute/GradientDomain/Utils.hlsli"

Texture2D<float4> gSource : register(t0);
RWTexture2D<float4> gDestXF : register(u0);
RWTexture2D<float4> gDestXB : register(u1);
RWTexture2D<float4> gDestYF : register(u2);
RWTexture2D<float4> gDestYB : register(u3);

[numthreads(16, 16, 1)]
void CSMain(uint3 DTid : SV_DispatchThreadID)
{
    uint2 pixelCoord = DTid.xy;

    uint width, height;
    gDestXF.GetDimensions(width, height);

    if (pixelCoord.x >= width || pixelCoord.y >= height)
        return;

    float3 c0 = gSource[pixelCoord].rgb;
    float3 cXF = SampleSafeMirror(gSource, c0, pixelCoord + uint2(1,0), 0, width, height);
    float3 cXB = SampleSafeMirror(gSource, c0, pixelCoord + uint2(-1,0), 0, width, height);
    float3 cYF = SampleSafeMirror(gSource, c0, pixelCoord + uint2(0,1), 0, width, height);
    float3 cYB = SampleSafeMirror(gSource, c0, pixelCoord + uint2(0,-1), 0, width, height);

    float mis = 0.5f;

    float3 gradientXF = (cXF - c0) * mis;
    float3 gradientXB = (c0 - cXB) * mis;
    float3 gradientYF = (cYF - c0) * mis;
    float3 gradientYB = (c0 - cYB) * mis;

    gDestXF[pixelCoord].rgb = gradientXF;
    gDestXB[pixelCoord].rgb = gradientXB;
    gDestYF[pixelCoord].rgb = gradientYF;
    gDestYB[pixelCoord].rgb = gradientYB;
}