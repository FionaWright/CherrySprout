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
// TODO: Should this use geometric normals?
ShiftResult ShiftHalfVector(ShadingFrame sframeMain, ShadingFrame sframeShifted, float3 wiMain, float3 woMain, float3 woShifted, float etaMain, float etaShifted)
{
    ShiftResult result = (ShiftResult)0;

    // TODO: Can be handled as special case
    if (!FEAT_CORE(GdptShiftHalf) || etaMain == 1.0f || etaShifted == 1.0f)
    {
        result.IsSuccessful = false;
        DBG_OUTPUT1(1.0f, GD_RejectionShiftMappingHalfVec);
        return result;
    }

    float3 V_s = sframeMain.ToLocal(woMain);
    float3 V_s_shifted = sframeShifted.ToLocal(woShifted);
    float3 L_s = sframeMain.ToLocal(wiMain);

    float NdV = SSpaceCosTheta(V_s);
    float NdL = SSpaceCosTheta(L_s);

    float3 L_s_shifted;

    bool isRefraction = NdV * NdL < 0.0f;

    if (FEAT_CORE(Transmission) && isRefraction)
    {
        float3 H_s = L_s + V_s * etaMain; // TODO: Probably wrong
        H_s = normalize(H_s);

        L_s_shifted = refract(-V_s_shifted, H_s, etaShifted);

        float3 H_s_shifted = L_s_shifted + V_s_shifted * etaShifted; // TODO: Here too
        //H_s_shifted = normalize(H_s_shifted); // ?

        float HLength2 = LengthSquared(H_s_shifted) / max(EPSILON, LengthSquared(H_s));
        float VdH = abs(dot(V_s, H_s)) / max(EPSILON, abs(dot(V_s_shifted, H_s_shifted)));
        result.Jacobian = HLength2 * VdH;
    }
    else // Reflection
    {
        float3 H_s = normalize(L_s + V_s);

        float3 N_s = float3(0,0,1);
        L_s_shifted = NormalizeSafe(reflect(-V_s_shifted, H_s), N_s);

        float VdH = abs(dot(V_s_shifted, H_s)) / max(EPSILON, abs(dot(V_s, H_s)));
        result.Jacobian = abs(VdH);
    }

    result.IsSuccessful = true;
    result.Wi = sframeShifted.ToWorld(L_s_shifted);
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