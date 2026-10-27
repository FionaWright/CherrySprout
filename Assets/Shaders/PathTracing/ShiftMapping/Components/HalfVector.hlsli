#ifndef H_SHIFT_MAPS_HALF_VEC_H
#define H_SHIFT_MAPS_HALF_VEC_H

// https://github.com/gradientpm/gradient-mts/blob/master/src/integrators/gradient/gpt/shift_mapping/shiftmapping.h

#include "PathTracing/ShiftMapping/Structs.hlsli"

// Is this eta relative or n?
ShiftResult ShiftHalfVector(float3 x1_Ng, float3 y1_Ng, float3 x1_wi, float3 x1_wo, float3 y1_wo, float etaMain, float etaShifted)
{
    ShiftResult result = (ShiftResult)0;

    // TODO: Can be handled as special case
    if (!FEAT_CORE(GdptShiftHalf) || etaMain == 1.0f || etaShifted == 1.0f)
    {
        result.IsSuccessful = false;
        DBG_OUTPUT1(1.0f, GD_RejectionShiftMappingHalfVec);
        return result;
    }

    ShadingFrame x_frame = CreateShadingFrame(x1_Ng);
    ShadingFrame y_frame = CreateShadingFrame(y1_Ng);

    float3 V_x = x_frame.ToLocal(x1_wo);
    float3 V_y = y_frame.ToLocal(y1_wo);
    float3 L_x = x_frame.ToLocal(x1_wi);

    float NdV = SSpaceCosTheta(V_x);
    float NdL = SSpaceCosTheta(L_x);

    float3 L_y;

    bool isRefraction = NdV * NdL < 0.0f;

    if (FEAT_CORE(Transmission) && isRefraction)
    {
        float3 H_s = L_x + V_x * etaMain; // TODO: Probably wrong
        H_s = normalize(H_s);

        L_y = refract(-V_y, H_s, etaShifted);

        float3 H_s_shifted = L_y + V_y * etaShifted; // TODO: Here too
        //H_s_shifted = normalize(H_s_shifted); // ?

        float HLength2 = LengthSquared(H_s_shifted) / max(EPSILON, LengthSquared(H_s));
        float VdH = abs(dot(V_x, H_s)) / max(EPSILON, abs(dot(V_y, H_s_shifted)));
        result.Jacobian = HLength2 * VdH;
    }
    else // Reflection
    {
        float3 H_x = normalize(L_x + V_x);

        float3 N_y = float3(0,0,1);
        L_y = NormalizeSafe(reflect(-V_y, H_x), N_y);

        if (L_y.z <= 0.0f)
        {
            result.IsSuccessful = false;
            return result;
        }

        float VdH = abs(dot(V_y, H_x)) / max(EPSILON, abs(dot(V_x, H_x)));
        result.Jacobian = abs(VdH);
    }

    result.IsSuccessful = true;
    result.Wi = y_frame.ToWorld(L_y);
    return result;
}

#endif