#include "Utils/CBVs.h"
#include "Utils/HlslUtils.hlsli"

Texture2D<float4> gSource : register(t0);
RWTexture2D<float4> gDest_4Channel : register(u0);
RWTexture2D<float>  gDest_1Channel : register(u1);

ConstantBuffer<CbvTextureConvert> gCBV : register(b0);

[numthreads(16, 16, 1)]
void CSMain(uint3 DTid : SV_DispatchThreadID)
{
    uint2 pixelCoord = DTid.xy;

    uint width, height;
    gDest_4Channel.GetDimensions(width, height);

    if (pixelCoord.x >= width || pixelCoord.y >= height)
        return;

    float4 sourceVal = gSource[pixelCoord];

    switch (gCBV.Mode)
    {

    case eNormalChannels2To3:
    {
        float2 normalXY = sourceVal.xy;
        normalXY = RemapUtoS(normalXY);

        float z = sqrt(max(0.0f, 1.0f - normalXY.x * normalXY.x - normalXY.y * normalXY.y));

        float3 normalXYZ = normalize(float3(normalXY, z));
        normalXYZ = RemapStoU(normalXYZ);

        gDest_4Channel[pixelCoord].xyz = normalXYZ;
        break;
    }

    case eOgmSplit:
    {
        float glossiness = sourceVal.y;
        float roughness = 1.0f - glossiness;
        float metallic = sourceVal.z;

        gDest_4Channel[pixelCoord].xyz = roughness;
        gDest_1Channel[pixelCoord].x = metallic;
        break;
    }

    case eOrmSplit:
    {
        float roughness = sourceVal.y;
        float metallic = sourceVal.z;

        gDest_4Channel[pixelCoord].xyz = roughness;
        gDest_1Channel[pixelCoord].x = metallic;
        break;
    }

    }
}