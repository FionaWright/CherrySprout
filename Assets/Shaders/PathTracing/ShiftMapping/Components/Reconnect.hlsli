#ifndef H_SHIFT_MAPS_RECONNECT_H
#define H_SHIFT_MAPS_RECONNECT_H

// https://github.com/gradientpm/gradient-mts/blob/master/src/integrators/gradient/gpt/shift_mapping/shiftmapping.h

#include "PathTracing/VisibilityRays.hlsli"

ShiftResult ShiftReconnect(float3 x1, float3 y1, float3 y1_Ng, float3 x2, float3 x2_Ng)
{
    ShiftResult result;

    if (!FEAT_CORE(GdptShiftRec))
    {
        result.IsSuccessful = false;
        return result;
    }

    float3 x1_x2 = x1 - x2;
    float3 y1_x2 = y1 - x2;

    float3 wiX = normalize(-x1_x2);
    float3 wiY = normalize(-y1_x2);

    float NdL_Y = dot(y1_Ng, wiY);
    if (NdL_Y <= 0.0f)
    {
        result.IsSuccessful = false;
        DBG_OUTPUT1(1.0f, GD_RejectionShiftMappingRecNdL);
        return result;
    }

    float lenSqrX = LengthSquared(x1_x2);
    float lenSqrY = LengthSquared(y1_x2);

    float cosTY = abs(dot(-wiY, x2_Ng));
    float cosTX = abs(dot(-wiX, x2_Ng));

    if (cosTX <= 1e-6f || lenSqrY <= 1e-12f)
    {
        result.IsSuccessful = false;
        DBG_OUTPUT1(1.0f, GD_RejectionShiftMappingRecNdL);
        return result;
    }

    result.IsSuccessful = true;
    result.Wi = wiY;
    result.Jacobian = (cosTY / cosTX) * (lenSqrX / lenSqrY);
    return result;
}

ShiftResult ShiftReconnectVis(float3 x1, float3 y1, float3 y1_Ng, float3 x2, float3 x2_Ng)
{
    ShiftResult result;

    if (!FEAT_CORE(GdptShiftRec))
    {
        result.IsSuccessful = false;
        return result;
    }

    bool occluded;
    TestVisibilityPoints(y1, x2, occluded);

    if (occluded)
    {
        result.IsSuccessful = false;
        DBG_OUTPUT1(1.0f, GD_RejectionShiftMappingRecOcc);
        return result;
    }

    return ShiftReconnect(x1, y1, y1_Ng, x2, x2_Ng);
}

#endif