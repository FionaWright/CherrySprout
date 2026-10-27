#ifndef H_SHIFT_MAPS_HYBRID_H
#define H_SHIFT_MAPS_HYBRID_H

#include "PathTracing/ShiftMapping/Structs.hlsli"
#include "PathTracing/ShiftMapping/Components/Reconnect.hlsli"
#include "PathTracing/ShiftMapping/Components/ReconnectEnv.hlsli"

ShiftResult ShiftMapHybrid(
    inout ReconnectionState reconnectionState,
    HitInfo hitInfo,
    PathVertexInfo v1,
    PathVertexInfo v2,
    float3 wo,
    uint raySegmentIdx,
    hlsl::uint recIdx,
    out BxdfEval bxdfEval)
{
    bool v1Dirac = MaterialIsDirac(v1.Roughness);
    bool isDirac = MaterialIsDirac(hitInfo.Mat.Roughness);

    DBG_OUTPUT1(isDirac, GD_IsLobeDirac);

    if (v1.PDF <= 0.0f || (v1Dirac != isDirac) || (MaterialIsDirac(v2.Roughness) && v2.Type == VertexType::eStandard))
    {
        DBG_OUTPUT1(1.0f, GD_RejectionDirac);

        ShiftResult shiftResult = (ShiftResult)0;
        shiftResult.IsSuccessful = false;
        return shiftResult;
    }

    bool isV2Reconnection = raySegmentIdx == recIdx;
    if (isV2Reconnection)
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
            bxdfEval = bxdf.Evaluate(hitInfo, wo, shiftResult.Wi, v1.Lobe);

        return shiftResult;
    }

    return ShiftRandomReplay(v1.RNG, hitInfo, wo, bxdfEval);
}


#endif