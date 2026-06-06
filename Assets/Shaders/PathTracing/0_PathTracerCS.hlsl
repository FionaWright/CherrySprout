#include "PathTracing/1_Core.hlsli"

[numthreads(16,16,1)]
void CSMain(uint3 DTid : SV_DispatchThreadID)
{
    if (DTid.x > gSettings.FrameDimensions.x || DTid.y > gSettings.FrameDimensions.y)
        return;

    Core(DTid.xy);
    //gTexOutput[DTid.xy].rgb = float3(DTid.xy / (float2)gSettings.FrameDimensions.xy, 0);
    //gTexAccumulation[DTid.xy].rgb = COLOR_GREEN;
}