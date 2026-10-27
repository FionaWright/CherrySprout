#ifndef H_SHIFT_MAPS_HR_H
#define H_SHIFT_MAPS_HR_H

#include "PathTracing/ShiftMapping/Structs.hlsli"
#include "PathTracing/ShiftMapping/Components/HalfVector.hlsli"
#include "PathTracing/ShiftMapping/Components/Reconnect.hlsli"
#include "PathTracing/ShiftMapping/Components/ReconnectEnv.hlsli"

ShiftResult ShiftMapHPlusR(
    inout ReconnectionState reconnectionState,
    HitInfo hitInfo,
    PathVertexInfo v1,
    PathVertexInfo v2,
    float3 wo)
{
    //bool v1Dirac = IsDirac(v1.Lobe, v1.Roughness);
    bool v1Dirac = MaterialIsDirac(v1.Roughness);
    bool isDirac = MaterialIsDirac(hitInfo.Mat.Roughness);

    DBG_OUTPUT1(isDirac, GD_IsLobeDirac);

    // TODO: When wi invalid, PDF is set to 1. Maybe should output 0 instead for stuff like this?
    if (v1.PDF <= 0.0f || (v1Dirac != isDirac) || (MaterialIsDirac(v2.Roughness) && v2.Type == VertexType::eStandard))
    {
        DBG_OUTPUT1(1.0f, GD_RejectionDirac);

        ShiftResult shiftResult = (ShiftResult)0;
        shiftResult.IsSuccessful = false;
        return shiftResult;
    }

    if (isDirac)
    {
        float iorNCurrent =  hitInfo.IsEntering ? IOR_N_AIR          : hitInfo.Mat.IOR_N;
        float iorNNext =     hitInfo.IsEntering ? hitInfo.Mat.IOR_N  : IOR_N_AIR;
        float eta = iorNCurrent / iorNNext;

        return ShiftHalfVector(v1.Ng_ff, hitInfo.Ng_ff, v1.Wi, v1.Wo, wo, v1.Eta, eta);
    }

    reconnectionState = ReconnectionState::eSemiConnected;

    if (v2.Type == VertexType::eStandard || v2.Type == VertexType::eEmission)
        return ShiftReconnectVis(v1.Position, hitInfo.HitPosBiased, hitInfo.Ng_ff, v2.Position, v2.Ng_ff);

    if (v2.Type == VertexType::eEnvironment)
        return ShiftReconnectEnvironmentVis(hitInfo.HitPosBiased, hitInfo.Ng_ff, v1.Wi);

    DBG_ASSERT_FAIL(GD_INVALID_VERTEX_TYPE);

    ShiftResult shiftResult = (ShiftResult)0;
    shiftResult.IsSuccessful = false;
    return shiftResult;
}


#endif