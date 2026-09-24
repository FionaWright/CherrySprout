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
    float3 cXF = gSource[pixelCoord + uint2(1,0)].rgb;
    float3 cXB = gSource[pixelCoord + uint2(-1,0)].rgb;
    float3 cYF = gSource[pixelCoord + uint2(0,1)].rgb;
    float3 cYB = gSource[pixelCoord + uint2(0,-1)].rgb;

    float3 gradientXF = cXF - c0;
    float3 gradientXB = c0 - cXB;
    float3 gradientYF = cYF - c0;
    float3 gradientYB = c0 - cYB;

    gDestXF[pixelCoord].rgb = gradientXF;
    gDestXB[pixelCoord].rgb = gradientXB;
    gDestYF[pixelCoord].rgb = gradientYF;
    gDestYB[pixelCoord].rgb = gradientYB;
}