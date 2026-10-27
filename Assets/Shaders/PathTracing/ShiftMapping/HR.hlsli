#ifndef H_SHIFT_MAPS_HR_H
#define H_SHIFT_MAPS_HR_H

#include "PathTracing/ShiftMapping/Structs.hlsli"
#include "PathTracing/ShiftMapping/Components/Fail.hlsli"
#include "PathTracing/ShiftMapping/Components/HalfVector.hlsli"
#include "PathTracing/ShiftMapping/Components/Reconnect.hlsli"
#include "PathTracing/ShiftMapping/Components/ReconnectEnv.hlsli"

ShiftResult ShiftMapHPlusR(
    inout ReconnectionState reconnectionState,
    HitInfo hitInfo,
    PathVertexInfo v1,
    PathVertexInfo v2,
    float3 wo,
    uint raySegmentIdx,
    out BxdfEval bxdfEval)
{
    bool v1Dirac = MaterialIsDirac(v1.Roughness);
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

    ShiftResult shiftResult;

    if (isDirac)
    {
        float eta = ComputeEtaAir(hitInfo.IsEntering, hitInfo.Mat.IOR_N);

        shiftResult = ShiftHalfVector(v1.Ng_ff, hitInfo.Ng_ff, v1.Wi, v1.Wo, wo, v1.Eta, eta);
    }
    else
    {
        reconnectionState = ReconnectionState::eSemiConnected;

        if (v2.Type == VertexType::eStandard || v2.Type == VertexType::eEmission)
            shiftResult = ShiftReconnectVis(v1.Position, hitInfo.HitPosBiased, hitInfo.Ng_ff, v2.Position, v2.Ng_ff);
        else if (v2.Type == VertexType::eEnvironment)
            shiftResult = ShiftReconnectEnvironmentVis(hitInfo.HitPosBiased, hitInfo.Ng_ff, v1.Wi);
        else
            shiftResult = ShiftFail();
    }

    if (shiftResult.IsSuccessful)
        bxdfEval = BxDF::Evaluate(hitInfo, wo, shiftResult.Wi, v1.Lobe);

    return shiftResult;
}


#endif