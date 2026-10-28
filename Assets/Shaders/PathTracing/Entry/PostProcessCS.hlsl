#include "Utils/CBVs.h"
#include "PathTracing/Utils.hlsli"
#include "PostProcessing/ToneMapping.hlsli"

Texture2D<float4> gSource : register(t0);
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

    // Exposure
    {
        v *= gCBV.Exposure;
    }

    // Tone-Mapping
    if (FEAT_CORE(ToneMapping))
    {
        TMO_ReinhardExt tmo;
        tmo.MaxWhite = gCBV.ToneMapWhitePoint;
        ToneMapJodie(v, tmo);

        //TMO_Aces tmo;
        //ToneMap(v, tmo);
    }

    // Gamma-Correct
    {
        v = LRGB_to_SRGB(v);
    }

    gDest[pixelCoord].rgb = v;
}