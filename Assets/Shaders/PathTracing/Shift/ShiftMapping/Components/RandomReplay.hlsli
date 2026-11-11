#ifndef H_SHIFT_MAPS_RANDOM_REPLAY_H
#define H_SHIFT_MAPS_RANDOM_REPLAY_H

#include "PathTracing/Shift/ShiftMapping/Structs.hlsli"

ShiftResult ShiftRandomReplay(RngValuesAncillary::RngValuesBxDF rng, HitInfo hitInfo, float3 wo, out BxdfEval bxdfEval)
{
    ShiftResult shiftResult = INIT0(ShiftResult);

    if (!FEAT_CORE(GdptShiftHalf)) // TODO: Use separate feat flag or remove
    {
        shiftResult.IsSuccessful = false;
        DBG_OUTPUT1(1.0f, GD_RejectionShiftMappingHalfVec);
        return shiftResult;
    }

    BxdfSample bxdfSample = BxDF::Sample(rng, hitInfo, wo, bxdfEval);

    shiftResult.IsSuccessful = (bxdfEval.PDF > 0.0f);
    shiftResult.Wi = bxdfSample.Wi;
    shiftResult.Jacobian = 1.0f;

    return shiftResult;
}

#endif