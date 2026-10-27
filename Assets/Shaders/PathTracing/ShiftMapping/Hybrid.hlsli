#ifndef H_SHIFT_MAPS_HYBRID_H
#define H_SHIFT_MAPS_HYBRID_H

#include "PathTracing/ShiftMapping/Structs.hlsli"
#include "PathTracing/ShiftMapping/Components/Reconnect.hlsli"
#include "PathTracing/ShiftMapping/Components/ReconnectEnv.hlsli"
#include "PathTracing/ShiftMapping/Components/RandomReplay.hlsli"

ShiftResult ShiftMapHybrid(
    inout ReconnectionState reconnectionState,
    HitInfo hitInfo,
    PathVertexInfo v1,
    PathVertexInfo v2,
    float3 wo,
    uint raySegmentIdx,
    out BxdfEval bxdfEval)
{
    bool v1Dirac = MaterialIsDirac(v1.Roughness);
    bool v2Dirac = MaterialIsDirac(v2.Roughness);
    bool isDirac = MaterialIsDirac(hitInfo.Mat.Roughness);

    DBG_OUTPUT1(isDirac, GD_IsLobeDirac);

    if (v2.Type == VertexType::eUninitialized)
    {
        DBG_OUTPUT1(1.0f, GD_RejectionUninit);

        ShiftResult shiftResult = (ShiftResult)0;
        shiftResult.IsSuccessful = false;
        return shiftResult;
    }

    if (v1.PDF <= 0.0f || (v1Dirac != isDirac))
    {
        DBG_OUTPUT1(1.0f, GD_RejectionDirac);

        ShiftResult shiftResult = (ShiftResult)0;
        shiftResult.IsSuccessful = false;
        return shiftResult;
    }

    // TODO: Some of these conditions can be computed earlier on the main path to avoid work
    // TODO: Jacobian denominator can also be computed earlier

#define DISTANCE_THRESHOLD 1.0f // TODO

    bool isValidReconnection = true;
    isValidReconnection &= v2.Type == VertexType::eEnvironment || LengthSquared(v2.Position - v1.Position) >= DISTANCE_THRESHOLD;
    isValidReconnection &= v2.Type == VertexType::eEnvironment || LengthSquared(v2.Position - hitInfo.HitPos) >= DISTANCE_THRESHOLD;
    //isValidReconnection &= !isDirac && !v1Dirac && !v2Dirac; // TODO: Assuming dirac is the threshold they're looking for
    isValidReconnection &= !isDirac && !v1Dirac;

    if (isValidReconnection)
    {
        reconnectionState = ReconnectionState::eSemiConnected;

        ShiftResult shiftResult;

        if (v2.Type == VertexType::eStandard || v2.Type == VertexType::eEmission)
            shiftResult = ShiftReconnectVis(v1.Position, hitInfo.HitPosBiased, hitInfo.Ng_ff, v2.Position, v2.Ng_ff);
        else if (v2.Type == VertexType::eEnvironment)
            shiftResult = ShiftReconnectEnvironmentVis(hitInfo.HitPosBiased, hitInfo.Ng_ff, v1.Wi);
        else
            shiftResult = ShiftFail();

        if (shiftResult.IsSuccessful)
            bxdfEval = BxDF::Evaluate(hitInfo, wo, shiftResult.Wi, v1.Lobe);

        return shiftResult;
    }

    return ShiftRandomReplay(v1.RNG.BxDF, hitInfo, wo, bxdfEval);
}


#endif