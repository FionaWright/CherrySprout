#include "Utils/SharedUtils.h"
#include "Utils/CBVs.h"

#include "Scene/PunctualLight.h"
#include "Utils/Math/Punctual.hlsli"

StructuredBuffer<PunctualLight> gPunctualLights : register(t0);
RWStructuredBuffer<float> gPDF  : register(u0);

[numthreads(64,1,1)]
void CSMain(uint3 DTid : SV_DispatchThreadID)
{
    uint punctualLightCount;
    uint punctualLightStride;
    gPunctualLights.GetDimensions(punctualLightCount, punctualLightStride);

    if (DTid.x >= punctualLightCount)
        return;

    PunctualLight light = gPunctualLights[DTid.x];
    float weight = PunctualLightWeight(light);
    gPDF[DTid.x] = weight;
}
