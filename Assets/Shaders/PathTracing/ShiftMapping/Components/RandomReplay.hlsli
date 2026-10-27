#ifndef H_SHIFT_MAPS_RANDOM_REPLAY_H
#define H_SHIFT_MAPS_RANDOM_REPLAY_H

#include "PathTracing/ShiftMapping/Structs.hlsli"

ShiftResult ShiftRandomReplay(RngValuesAncillary rng, HitInfo hitInfo, float3 wo, out BxdfEval bxdfEval)
{
    BxDF bxdf;
    BxdfSample bxdfSample = bxdf.Sample(rng, hitInfo, wo, bxdfEval);

    ShiftResult shiftResult = (ShiftResult)0;

    shiftResult.IsSuccessful = (bxdfEval.PDF > 0.0f);
    shiftResult.Wi = bxdfSample.Wi;
    shiftResult.Jacobian = 1.0f; // TODO

    return shiftResult;
}

#endif