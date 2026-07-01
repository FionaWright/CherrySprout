#ifndef H_TARGET_H
#define H_TARGET_H

struct LightSampleSelection
{
    float3 ScaledDirection; // Direction * Distance
    uint LightIndex;

    float3 Radiance;
    float p;
};

void ReSTIR_DI(uint2 pixelCoord)
{
    uint reservoirIdx = GetReservoirBufferIndex_Current(pixelCoord);
}

#endif