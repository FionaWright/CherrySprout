#include "Utils/CBVs.h"
#include "PathTracing/Utils.hlsli"
#include "PostProcessing/PostProcess.hlsli"

Texture2D<float4> gSource : register(t0);
Texture2D<float4> gBloom : register(t1);
RWTexture2D<float4> gDest : register(u0);

ConstantBuffer<CbvPostProcessing> gCBV : register(b0);

[numthreads(16, 16, 1)]
void CSMain(uint3 DTid : SV_DispatchThreadID)
{
    uint2 pixelCoord = DTid.xy;

    uint width, height;
    gDest.GetDimensions(width, height);

    if (pixelCoord.x >= width || pixelCoord.y >= height)
        return;

    float3 v = gSource[pixelCoord].rgb;
    v += gBloom[pixelCoord].rgb;

    v = PostProcess(v, gCBV.Exposure, gCBV.ToneMapWhitePoint, true);

    gDest[pixelCoord].rgb = v;
}