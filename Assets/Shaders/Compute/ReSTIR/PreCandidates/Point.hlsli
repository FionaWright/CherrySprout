#ifndef H_PIXELINFO_POINT_H
#define H_PIXELINFO_POINT_H

PreCandidatePoint GeneratePreCandidate(inout RngInfo rngInfo, ContextSSPoint context)
{
    return INIT0(PreCandidatePoint);
}

template <typename TReservoir, typename TPreCandidate>
bool TryFindTemporalReservoir(
    inout RngInfo rngInfo,

    RWStructuredBuffer< TReservoir > buffer,
    ContextSSPoint context,
    TPreCandidate _,
    uint2 pixelCoord,
    float2 mv,

    out int2 prevPixelCoord,
    out TReservoir temporalReservoir)
{
    prevPixelCoord = MotionVectorToPixelCoord(pixelCoord, mv, gSettings.FrameDimensions);

    bool validPrevPixel = all(prevPixelCoord >= 0) && all(prevPixelCoord < int2((float2)gSettings.FrameDimensions));
    if (!validPrevPixel)
        return false;

    uint reservoirIdxPrevIn = GetReservoirBufferIndex_Previous(prevPixelCoord);
    temporalReservoir = buffer[reservoirIdxPrevIn];
    return true;
}

#endif