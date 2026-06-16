#include "Utils/SharedUtils.h"

Texture2D<float4> gTex : register(t0);
RWStructuredBuffer<float> gSumLum : register(u0);

#define BLOCK_SIZE              9
#define WARP_SIZE_1D            32
#define WARP_SIZE               WARP_SIZE_1D*WARP_SIZE_1D
#define THREAD_GROUP_COVERAGE   WARP_SIZE_1D*BLOCK_SIZE

groupshared float localSumLum[WARP_SIZE];

[numthreads(WARP_SIZE_1D, WARP_SIZE_1D, 1)]
void CSMain(
    uint3 DTid : SV_DispatchThreadID,
    uint3 GTid : SV_GroupThreadID,
    uint3 Gid  : SV_GroupID)
{
    uint2 dim;
    gTex.GetDimensions(dim.x, dim.y);

    uint threadIdx = GTid.y * WARP_SIZE_1D + GTid.x;

    float sumLum = 0.0f;

    [unroll]
    for (int y = 0; y < BLOCK_SIZE; y++)
        [unroll]
        for (int x = 0; x < BLOCK_SIZE; x++)
        {
            uint2 pixel = DTid.xy * BLOCK_SIZE + uint2(x, y);

            if (pixel.x > dim.x || pixel.y > dim.y)
                continue;

            float3 color = gTex.Load(uint3(pixel, 0)).rgb;
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
    uint groupsX = (dim.x + THREAD_GROUP_COVERAGE - 1) / THREAD_GROUP_COVERAGE;
    uint outputIdx = Gid.y * groupsX + Gid.x;

    gSumLum[outputIdx] = sumLum;
}