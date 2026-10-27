#ifndef H_SHIFT_MAPS_RECONNECT_ENV_H
#define H_SHIFT_MAPS_RECONNECT_ENV_H

// https://github.com/gradientpm/gradient-mts/blob/master/src/integrators/gradient/gpt/shift_mapping/shiftmapping.h

#include "PathTracing/ShiftMapping/Structs.hlsli"
#include "PathTracing/VisibilityRays.hlsli"

ShiftResult ShiftReconnectEnvironment(float3 y1_Ng, float3 dirMain)
{
    ShiftResult result;

    if (!FEAT_CORE(GdptShiftRecEnv))
    {
        result.IsSuccessful = false;
        return result;
    }

    float NdL = dot(y1_Ng, dirMain);
    if (NdL <= 0.0f)
    {
        result.IsSuccessful = false;
        DBG_OUTPUT1(1.0f, GD_RejectionShiftMappingEnvNdL);
        return result;
    }

    result.IsSuccessful = true;
    result.Jacobian = 1.0f;
    result.Wi = dirMain;
    return result;
}

ShiftResult ShiftReconnectEnvironmentVis(float3 x1, float3 y1_Ng, float3 dirMain)
{
    ShiftResult result;

    if (!FEAT_CORE(GdptShiftRecEnv))
    {
        result.IsSuccessful = false;
        return result;
    }

    bool occluded;
    TestVisibilityDirection(x1, dirMain, occluded);

    if (occluded)
    {
        result.IsSuccessful = false;
        DBG_OUTPUT1(1.0f, GD_RejectionShiftMappingEnvOcc);
        return result;
    }

    return ShiftReconnectEnvironment(y1_Ng, dirMain);
}

#endif