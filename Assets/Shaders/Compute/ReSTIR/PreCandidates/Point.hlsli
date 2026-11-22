#ifndef H_PIXELINFO_POINT_H
#define H_PIXELINFO_POINT_H

PreCandidatePoint GeneratePreCandidate(inout RngInfo rngInfo, uint2 pixelCoord, ContextSSPoint context)
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

    out int2 selectedPixelCoord,
    out TReservoir selectedReservoir)
{
    selectedPixelCoord = MotionVectorToPixelCoord(pixelCoord, mv, gSettings.FrameDimensions);

    bool validPrevPixel = all(selectedPixelCoord >= 0) && all(selectedPixelCoord < int2((float2)gSettings.FrameDimensions));
    if (!validPrevPixel)
        return false;

    uint reservoirIdxPrevIn = GetReservoirBufferIndex_Previous(selectedPixelCoord);
    selectedReservoir = buffer[reservoirIdxPrevIn];
    return true;
}

#endif