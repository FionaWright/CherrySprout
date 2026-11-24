#ifndef H_PIXELINFO_AREA_H
#define H_PIXELINFO_AREA_H

#include "PathTracing/HitInfo/GetHitInfo.hlsli"
#include "PathTracing/3_GetPrimaryRay.hlsli"

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

    Area restir requires primary hits to be re-traced for each candidate, as such there is basically no context
    One idea is to have it so Area ReSTIR has no context and uses the existing pixel info system
    The other idea is to put that information into a new context and recompute context every candidate (Messy)

    I think keeping it in a pixel info inside the reservoir is a good idea, and then Area ReSTIR can basically just ignore the context which will be empty
*/

PreCandidateArea GeneratePreCandidate(inout RngInfo rngInfo, uint2 pixelCoord, ContextSSArea _)
{
    PreCandidateArea preCandidate = INIT0(PreCandidateArea);

    preCandidate.RngPrimary = SampleRngPrimary(rngInfo);

    float3 rayOrigin;
    GetPrimaryRay(
        preCandidate.RngPrimary, gSettings.CameraPositionWorld, gSettings.TexelSize,
        pixelCoord, gSettings.InvV, gSettings.InvP,
        gSettings.DofFocalDist, gSettings.DofLensRadius,
        rayOrigin, preCandidate.CenterRayDirection);

    PathState pathState = CreatePathState(rayOrigin, preCandidate.CenterRayDirection);

    RayQuery<RAY_FLAGS> q;
    q.TraceRayInline(gTLAS, RAY_FLAGS, 0xFF, pathState.Desc);
    while (q.Proceed()) {}

    preCandidate.IsMiss = q.CommittedStatus() != COMMITTED_TRIANGLE_HIT;

    if (!preCandidate.IsMiss)
        GetHitInfo(q, preCandidate.HitInfo);

    return preCandidate;
}

void ShiftPreCandidate(uint2 pixelCoord, inout PreCandidateArea preCandidate)
{
    // Same jitter sample, different pixel coord -> different ray direction / hitinfo / etc

    float3 rayOrigin;
    GetPrimaryRay(
        preCandidate.RngPrimary, gSettings.CameraPositionWorld, gSettings.TexelSize,
        pixelCoord, gSettings.InvV, gSettings.InvP,
        gSettings.DofFocalDist, gSettings.DofLensRadius,
        rayOrigin, preCandidate.CenterRayDirection);

    PathState pathState = CreatePathState(rayOrigin, preCandidate.CenterRayDirection);

    RayQuery<RAY_FLAGS> q;
    q.TraceRayInline(gTLAS, RAY_FLAGS, 0xFF, pathState.Desc);
    while (q.Proceed()) {}

    preCandidate.IsMiss = q.CommittedStatus() != COMMITTED_TRIANGLE_HIT;

    if (!preCandidate.IsMiss)
        GetHitInfo(q, preCandidate.HitInfo);
}

template <typename TReservoir, typename TPreCandidate>
bool TryFindTemporalReservoir(
    inout RngInfo rngInfo,

    RWStructuredBuffer< TReservoir > buffer,
    bool reservoirIsEmpty,
    ContextSSArea _,
    TPreCandidate preCandidate,
    uint2 pixelCoord,
    float2 mv,

    out int2 selectedPixelCoord,
    out TReservoir selectedReservoir)
{
    selectedPixelCoord = int2(-1,-1);

    if (reservoirIsEmpty)
    {
        selectedPixelCoord = MotionVectorToPixelCoord(pixelCoord, mv, gSettings.FrameDimensions);
        bool validPrevPixel = all(selectedPixelCoord >= 0) && all(selectedPixelCoord < int2((float2)gSettings.FrameDimensions));
        if (!validPrevPixel)
            return false;

        uint reservoirIdxPrevIn = GetReservoirBufferIndex_Previous(selectedPixelCoord);
        selectedReservoir = buffer[reservoirIdxPrevIn];
        return true;
    }

    float2 dimensions = gSettings.FrameDimensions;

    float2 currJitter = preCandidate.RngPrimary.Jitter - 0.5f;
    float2 pixelCenter = pixelCoord + 0.5f + currJitter;   // [0, WxH]
    float2 currUV = pixelCenter / dimensions; // [0, 1]
    float2 prevUV = currUV - mv;              // [0, 1]

    float2 supportCenter = prevUV * dimensions; // [0, WxH]
    float2 supportBase = supportCenter - 0.5f;
    float2 supportTop = supportBase + 1.0f;

    float2 pixel2x2Base = floor(supportBase);

    bool validPrevPixel = all(pixel2x2Base >= 0) && all(pixel2x2Base+1 < int2((float2)gSettings.FrameDimensions));
    if (!validPrevPixel)
        return false;

    uint2 sourcePixelCoords[4];
    TReservoir reservoirs[4];
    float pHats[4];
    float pdfs[4];

    float pdfSum = 0.0f;

    const uint2 OFFSETS[4] = {
        uint2(0,0),
        uint2(1,0),
        uint2(0,1),
        uint2(1,1)
    };

    [loop]
    for (uint i = 0; i < 4; i++)
    {
        uint2 coord = uint2(pixel2x2Base) + OFFSETS[i];
        uint reservoirIdx = GetReservoirBufferIndex_Previous(coord);
        reservoirs[i] = buffer[reservoirIdx];

        sourcePixelCoords[i] = coord;

        float2 candidateJitterFromCenter = reservoirs[i].PreCandidate.RngPrimary.Jitter - 0.5f;
        float2 candidatePosition = float2(coord) + 0.5f + candidateJitterFromCenter;

        if (FEAT_ADV(AreaRestirRobust))
        {
            int xShiftDirection =
                candidatePosition.x < supportBase.x ? 1 :
                candidatePosition.x > supportTop.x ? -1 : 0;

            int yShiftDirection =
                candidatePosition.y < supportBase.y ? 1 :
                candidatePosition.y > supportTop.y ? -1 : 0;

            // No shift needed
            if (xShiftDirection == 0 && yShiftDirection == 0)
            {
                pHats[i] = reservoirs[i].PHat;
                continue;
            }

            // Shift precandidates to be within support
            int2 shiftedPixelCoord = (int2)coord + int2(xShiftDirection, yShiftDirection);
            bool validShiftedPixel =
                all(shiftedPixelCoord >= supportBase) &&
                all(shiftedPixelCoord <= supportTop);

            if (!validShiftedPixel)
            {
                pHats[i] = 0.0f;
                continue;
            }

            sourcePixelCoords[i] = shiftedPixelCoord;

            ShiftPreCandidate(shiftedPixelCoord, reservoirs[i].PreCandidate);

            // Shift candidate using new precandidate
            Shift(reservoirs[i].Y, reservoirs[i].PreCandidate);

            // Recalculate pHat
            pHats[i] = Target(reservoirs[i].Y, reservoirs[i].PreCandidate);
        }
        else
        {
            bool rejectCandidate =
                any(candidatePosition < supportBase) ||
                any(candidatePosition > supportTop) ||
			    reservoirs[i].PreCandidate.IsMiss;

            if (rejectCandidate)
                pHats[i] = 0.0f;
            else
                pHats[i] = reservoirs[i].PHat;
        }
    }

    [unroll]
    for (uint i = 0; i < 4; i++)
    {
        pdfs[i] = PDF(reservoirs[i].Y);
        pdfSum += pdfs[i];
    }

    // TODO: Should this just use reservoir merging instead?

    // RIS
    {
        float w[4];
        float wSum = 0.0f;

        uint M = 4;

        [unroll]
        for (uint i = 0; i < M; i++)
        {
            //float mis = 1.0f / float(M); // TODO: MIS is balance heuristic of pHats according to paper for robust?
            float mis = pdfs[i] / pdfSum; // Note: Balance Heuristic
            w[i] = mis * pHats[i] * reservoirs[i].W_Y; // TODO: Correct to use W_Y?
            wSum += w[i];
        }

        if (wSum <= 0.0f)
            return false;

        float r = Rand01(rngInfo);
        for (uint i = 0; i < M; i++)
        {
            r -= w[i] / wSum;
            if (r <= 0.0f)
            {
                float W_Y = wSum / (float(M) * pHats[i]); // TODO: Do something with this?

                selectedPixelCoord = sourcePixelCoords[i];
                selectedReservoir = reservoirs[i];
                SetUCW(selectedReservoir);
                return true;
            }
        }
    }

    return false;
}

#endif