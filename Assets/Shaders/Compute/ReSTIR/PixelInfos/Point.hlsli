#ifndef H_PIXELINFO_POINT_H
#define H_PIXELINFO_POINT_H

PixelInfoPoint InitPixelInfo(inout RngInfo rngInfo, ContextScreenSpace context)
{
    RngValuesPrimary rngPrimary = SampleRngPrimary(rngInfo);

    float3 rayOrigin;
    float3 rayDirection;
    GetPrimaryRay(
        rngPrimary, gSettings.CameraPositionWorld, gSettings.TexelSize,
        pixelCoord, gSettings.InvV, gSettings.InvP,
        gSettings.DofFocalDist, gSettings.DofLensRadius,
        rayOrigin, rayDirection);

    PixelInfoPoint pixelInfo;
    pixelInfo.RayDirection = rayDirection;
    return pixelInfo;
}

template <typename TCandidate>
Reservoir<TCandidate> FindTemporalReservoir(PixelInfoPoint pixelInfo, RWStructuredBuffer< Reservoir<TCandidate> > buffer, uint2 pixelCoord, float2 mv)
{
    int2 prevPixelCoord = MotionVectorToPixelCoord(pixelCoord, mv, gSettings.FrameDimensions);

    bool validPrevPixel = all(prevPixelCoord >= 0) && all(prevPixelCoord < int2((float2)gSettings.FrameDimensions));
    if (!validPrevPixel)
    {
        buffer[reservoirIdxOut] = reservoir;
        TryPreserveReservoirTemporal(buffer, reservoir, pixelCoord);
        return;
    }

    uint reservoirIdxPrevIn = GetReservoirBufferIndex_Previous(prevPixelCoord);
    return buffer[reservoirIdxPrevIn];
}

#endif