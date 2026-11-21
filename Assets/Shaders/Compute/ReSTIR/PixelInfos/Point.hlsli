#ifndef H_PIXELINFO_POINT_H
#define H_PIXELINFO_POINT_H

PixelInfoPoint InitPixelInfo(inout RngInfo rngInfo, ContextScreenSpace context)
{
    PixelInfoPoint pixelInfo;
    return pixelInfo;
}

template <typename TCandidate>
ReservoirX FindTemporalReservoir(inout RngInfo rngInfo, PixelInfoPoint pixelInfo, RWStructuredBuffer< ReservoirX > buffer, uint2 pixelCoord, float2 mv)
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