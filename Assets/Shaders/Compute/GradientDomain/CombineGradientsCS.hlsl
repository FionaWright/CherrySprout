#include "Compute/GradientDomain/Utils.hlsli"

Texture2D<float4> gTexGradientXF : register(t0);
Texture2D<float4> gTexGradientXB : register(t1);
Texture2D<float4> gTexGradientYF : register(t2);
Texture2D<float4> gTexGradientYB : register(t3);

RWTexture2D<float4> gDestX : register(u0);
RWTexture2D<float4> gDestY : register(u1);

[numthreads(16, 16, 1)]
void CSMain(uint3 DTid : SV_DispatchThreadID)
{
    uint2 pixelCoord = DTid.xy;

    uint width, height;
    gDestX.GetDimensions(width, height);

    if (pixelCoord.x >= width || pixelCoord.y >= height)
        return;

    float3 gradientX = GetTrueGradientX(gTexGradientXF, gTexGradientXB, pixelCoord, width, height);
    float3 gradientY = GetTrueGradientY(gTexGradientYF, gTexGradientYB, pixelCoord, width, height);

    gDestX[pixelCoord].rgb = gradientX;
    gDestY[pixelCoord].rgb = gradientY;
}