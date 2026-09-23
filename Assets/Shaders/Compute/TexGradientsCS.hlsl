Texture2D<float4> gSource : register(t0);
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

    float3 c0 = gSource[pixelCoord].rgb;
    float3 cX = gSource[pixelCoord + uint2(1,0)].rgb;
    float3 cY = gSource[pixelCoord + uint2(0,1)].rgb;

    float3 gradientX = cX - c0;
    float3 gradientY = cY - c0;

    gDestX[pixelCoord].rgb = gradientX;
    gDestY[pixelCoord].rgb = gradientY;
}