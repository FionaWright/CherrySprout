#include "PathTracing/Utils.hlsli"

Texture2D<float4> gSource : register(t0);
RWTexture2D<float4> gDest : register(u0);

[numthreads(16, 16, 1)]
void CSMain(uint3 DTid : SV_DispatchThreadID)
{
    uint2 pixelCoord = DTid.xy;

    uint width, height;
    gDest.GetDimensions(width, height);

    if (pixelCoord.x >= width || pixelCoord.y >= height)
        return;

    gDest[pixelCoord].rgb = LRGB_to_SRGB(gSource[pixelCoord].rgb);
}