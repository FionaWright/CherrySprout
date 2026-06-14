#include "Utils/SharedUtils.h"

Texture2D<float4> gTex : register(t0);
RWStructuredBuffer<float> gSumLum : register(u0);

#define BLOCK_SIZE              9
#define WARP_SIZE_1D            32
#define WARP_SIZE               WARP_SIZE_1D*WARP_SIZE_1D
#define THREAD_GROUP_COVERAGE   WARP_SIZE_1D*BLOCK_SIZE

groupshared float localSumLum[WARP_SIZE];

[numthreads(WARP_SIZE_1D, WARP_SIZE_1D, 1)]
void CSMain(uint3 DTid : SV_DispatchThreadID)
{
    uint2 dim;
    gTex.GetDimensions(dim.x, dim.y);

    float2 texelSize = 1.0f / dim;

    uint2 threadID = fmod(DTid.xy, WARP_SIZE_1D);
    uint threadIdx = threadID.y * WARP_SIZE_1D + threadID.x;

    float sumLum = 0.0f;

    [unroll]
    for (int y = 0; y < BLOCK_SIZE; y++)
        [unroll]
        for (int x = 0; x < BLOCK_SIZE; x++)
        {
            float2 uvXY = float2(DTid.xy * BLOCK_SIZE) + float2(x, y);
            float3 color = gTex.Load(int3(uvXY, 0)).rgb;

            float lum = Luminance(color);
            sumLum += lum;
        }

    localSumLum[threadIdx] = sumLum;

    GroupMemoryBarrierWithGroupSync();
    if (threadIdx != 0)
        return;

    for (int i = 1; i < WARP_SIZE; i++)
    {
        sumLum += localSumLum[i];
    }

    // Assuming square texture
    uint numThreadGroups = dim.x / THREAD_GROUP_COVERAGE;

    uint2 groupID = DTid.xy / WARP_SIZE_1D;
    uint outputIdx = groupID.y * numThreadGroups + groupID.x;

    gSumLum[outputIdx] = sumLum;
}