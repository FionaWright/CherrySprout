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
ReservoirX_Area FindTemporalReservoir(inout RngInfo rngInfo, PixelInfoArea pixelInfo, RWStructuredBuffer< ReservoirX_Area > buffer, uint2 pixelCoord, float2 mv)
{
    float2 dimensions = gSettings.FrameDimensions;

    float2 pixelUV = pixelCoord + 0.5f;
    float2 currUV = pixelUV / dimensions;
    float2 prevUV = currUV - mv;

    float2 supportCenter = prevUV * dimensions;
    float2 supportBase = supportCenter - 0.5f;
    float2 supportTop = supportBase + 1.0f;

    float2 pixel2x2Base = floor(supportBase);

    bool validPrevPixel = all(pixel2x2Base >= 0) && all(pixel2x2Base < int2((float2)gSettings.FrameDimensions));
    if (!validPrevPixel)
    {
        buffer[reservoirIdxOut] = reservoir;
        TryPreserveReservoirTemporal(buffer, reservoir, pixelCoord);
        return;
    }

    float2 jitterFromCenter = pixelInfo.RngPrimary.Jitter - 0.5f;
    float2 samplePosition = supportCenter + jitterFromCenter;

    ReservoirX_Area samples[4];

    const uint2 OFFSETS[4] = {
        uint2(0,0),
        uint2(1,0),
        uint2(0,1),
        uint2(1,1)
    };

    if (FEAT_CORE(AreaRestirRobust)
    {
    }
    else
    {
        [unroll]
        for (int i = 0; i < 4; i++)
        {
            uint2 coord = uint2(pixel2x2Base) + OFFSETS[i];
            uint reservoirIdx = GetReservoirBufferIndex_Previous(prevPixelCoord);
            samples[i] = buffer[reservoirIdx];

            float2 candidateJitterFromCenter = samples[i].PixelInfo.RngPrimary.Jitter - 0.5f;
            float2 candidatePosition = float2(coord) + 0.5f + candidateJitterFromCenter;

            bool rejectCandidate =
                any(candidatePosition < supportBase) ||
                any(candidatePosition > supportTop);
            if (rejectCandidate)
            {
                samples[i].PHat = 0.0f;
            }
        }
    }

    // RIS
    {
        [unroll]
        for (int i = 0; i < 4; i++)
        {
            uint2 coord = uint2(pixel2x2Base) + OFFSETS[i];
            uint reservoirIdx = GetReservoirBufferIndex_Previous(prevPixelCoord);
            samples[i] = buffer[reservoirIdx];

            float2 candidateJitterFromCenter = samples[i].PixelInfo.RngPrimary.Jitter - 0.5f;
            float2 candidatePosition = float2(coord) + 0.5f + candidateJitterFromCenter;

            bool rejectCandidate =
                any(candidatePosition < supportBase) ||
                any(candidatePosition > supportTop);
            if (rejectCandidate)
            {
                samples[i].PHat = 0.0f;
            }
        }
    }

    uint reservoirIdxPrevIn = GetReservoirBufferIndex_Previous(prevPixelCoord);
    return buffer[reservoirIdxPrevIn];
}

#endif