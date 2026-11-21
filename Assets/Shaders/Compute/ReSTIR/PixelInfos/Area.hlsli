#ifndef H_PIXELINFO_AREA_H
#define H_PIXELINFO_AREA_H

/*
    TODO:
    Fast Method:
    - Backproject current sample using -mv
    - Iterate over each pixel reservoir within the support (pixel area)
    - If any samples are outside support (due to jitter/DoF), skip/reject them
    - Perform RIS on valid samples (mis = 1)
    - Forward project selected sample back to current frame (if found) using mv
    - (Rejected samples still increase confidence? (M))

    Robust Method:
    - Should be used most of the time
    - Instead of rejecting samples outside the support, shift them by one pixel (Both X and Y axis if required) in the direction of the backprojected sample
    - Now all samples will always be in the support
    - Requires using other pixel contexts and doing a Shift() I think
    - MIS weights use balance heuristic (Requires shifting one of the samples to the other 3 or something?)

    Wo / RngPrimary needs to be updatable candidate information, not context information from now on
    Given that then each candidate has its own jitter, do I need to do anything special to use it in the actual path? (DI)
*/

PixelInfoArea InitPixelInfo(inout RngInfo rngInfo, ContextScreenSpace context)
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
    pixelInfo.RngPrimary = rngPrimary;
    return pixelInfo;
}

template <typename TCandidate>
Reservoir<TCandidate> FindTemporalReservoir(PixelInfoArea pixelInfo, RWStructuredBuffer< Reservoir<TCandidate> > buffer, uint2 pixelCoord, float2 mv, float)
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