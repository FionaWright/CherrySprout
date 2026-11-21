#ifndef H_PIXELINFO_POINT_H
#define H_PIXELINFO_POINT_H

PreCandidatePoint GeneratePreCandidate(inout RngInfo rngInfo, ContextSSPoint context)
{
    return INIT0(PreCandidatePoint);
}

template <typename TCandidate>
bool TryFindTemporalReservoir(
    inout RngInfo rngInfo,

    RWStructuredBuffer< ReservoirX_Area > buffer,
    ContextSSPoint context,
    PreCandidateArea _,
    uint2 pixelCoord,
    float2 mv,

    out ReservoirX_Area temporalReservoir)
{
    int2 prevPixelCoord = MotionVectorToPixelCoord(pixelCoord, mv, gSettings.FrameDimensions);

    bool validPrevPixel = all(prevPixelCoord >= 0) && all(prevPixelCoord < int2((float2)gSettings.FrameDimensions));
    if (!validPrevPixel)
        return false;

    uint reservoirIdxPrevIn = GetReservoirBufferIndex_Previous(prevPixelCoord);
    temporalReservoir = buffer[reservoirIdxPrevIn];
    return true;
}

#endif