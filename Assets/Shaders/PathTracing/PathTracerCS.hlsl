#include "PathTracing/Core.hlsli"

[numthreads(16,16,1)]
void CSMain(uint3 DTid : SV_DispatchThreadID)
{
    if (DTid.x > gSettings.FrameDimensions.x || DTid.y > gSettings.FrameDimensions.y)
        return;

    Core(DTid.xy);
}