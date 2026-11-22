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

// TODO: Disable normal/depth rejection heuristics
// TODO: Requires new special shift mappings for first vertex?

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

    // TODO: Jacobian?

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
    ContextSSArea _,
    TPreCandidate preCandidate,
    uint2 pixelCoord,
    float2 mv,

    out int2 selectedPixelCoord,
    out TReservoir temporalReservoir)
{
    selectedPixelCoord = int2(-1,-1);

    float2 dimensions = gSettings.FrameDimensions;

    float2 pixelUV = pixelCoord + 0.5f;
    float2 currUV = pixelUV / dimensions;
    float2 prevUV = currUV - mv;

    float2 supportCenter = prevUV * dimensions;
    float2 supportBase = supportCenter - 0.5f;
    float2 supportTop = supportBase + 1.0f;

    float2 pixel2x2Base = floor(supportBase);

    bool validPrevPixel = all(pixel2x2Base >= 0) && all(pixel2x2Base+1 < int2((float2)gSettings.FrameDimensions));
    if (!validPrevPixel)
        return false;

    TReservoir samples[4];

    const uint2 OFFSETS[4] = {
        uint2(0,0),
        uint2(1,0),
        uint2(0,1),
        uint2(1,1)
    };

    if (FEAT_ADV(AreaRestirRobust))
    {
        // TODO
        // Remember to shift both candidate AND precandidate!
    }
    else
    {
        [unroll]
        for (uint i = 0; i < 4; i++)
        {
            uint2 coord = uint2(pixel2x2Base) + OFFSETS[i];
            uint reservoirIdx = GetReservoirBufferIndex_Previous(coord);
            samples[i] = buffer[reservoirIdx];

            float2 candidateJitterFromCenter = samples[i].PreCandidate.RngPrimary.Jitter - 0.5f;
            float2 candidatePosition = float2(coord) + 0.5f + candidateJitterFromCenter;

            bool rejectCandidate =
                any(candidatePosition < supportBase) ||
                any(candidatePosition > supportTop) ||
				samples[i].PreCandidate.IsMiss;
            if (rejectCandidate)
            {
                samples[i].PHat = 0.0f;
            }
        }
    }

    // RIS
    {
        float w[4];
        float wSum = 0.0f;

        uint M = 4;

        [unroll]
        for (uint i = 0; i < M; i++)
        {
            float mis = 1.0f;
            w[i] = mis * samples[i].PHat * samples[i].W_Y; // TODO: Correct to use W_Y?
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
                float W_Y = wSum / (float(M) * samples[i].PHat); // TODO: Do something with this?

                selectedPixelCoord = uint2(pixel2x2Base) + OFFSETS[i];
                uint reservoirIdxPrevIn = GetReservoirBufferIndex_Previous(selectedPixelCoord);
                temporalReservoir = buffer[reservoirIdxPrevIn];
                return true;
            }
        }
    }

    return false;
}

#endif