#ifndef H_RESTIR_THRESHOLDS_H
#define H_RESTIR_THRESHOLDS_H

bool PassNormalThreshold(float3 N1, float3 N2, float threshold)
{
    return dot(N1, N2) >= threshold;
}

bool PassDepthThreshold(float depthLinear1, float depthLinear2, float threshold)
{
    float denominator = max(1e-6f, min(depthLinear1, depthLinear2));

    float relativeDifference = abs(depthLinear1 - depthLinear2) / denominator;

    return relativeDifference <= threshold;
}

template <typename TReservoir>
void TryPreserveReservoirTemporal(RWStructuredBuffer<TReservoir> buffer, TReservoir reservoir, uint2 pixelCoord)
{
    if (gRestirSettings.SpatialEnabled) // Wait until after spatial pass
        return;

    uint idx = GetReservoirBufferIndex_Previous(pixelCoord);
    buffer[idx] = reservoir;
}

template <typename TReservoir>
void TryPreserveReservoirSpatial(RWStructuredBuffer<TReservoir> buffer, TReservoir reservoir, uint2 pixelCoord)
{
    if (!gRestirSettings.TemporalEnabled)
        return;

    uint idx = GetReservoirBufferIndex_Previous(pixelCoord);
    buffer[idx] = reservoir;
}

#endif