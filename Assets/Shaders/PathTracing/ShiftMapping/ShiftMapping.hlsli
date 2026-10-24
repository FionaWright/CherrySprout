#ifndef H_SHIFT_MAPPING_H
#define H_SHIFT_MAPPING_H

// https://github.com/gradientpm/gradient-mts/blob/master/src/integrators/gradient/gpt/shift_mapping/shiftmapping.h

#include "PathTracing/VisibilityRays.hlsli"

struct ShiftResult
{
    bool IsSuccessful;
    float Jacobian;
    float3 Wi;
};

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