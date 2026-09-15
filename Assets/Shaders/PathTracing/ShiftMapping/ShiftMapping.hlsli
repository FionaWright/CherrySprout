#ifndef H_SHIFT_MAPPING_H
#define H_SHIFT_MAPPING_H

// https://github.com/gradientpm/gradient-mts/blob/master/src/integrators/gradient/gpt/shift_mapping/shiftmapping.h

bool TraceVisibilityRay(float3 startPoint, float3 endPoint)
{
    // TraceShadowRay could be a wrapper over this function
}

bool TraceVisibilityRayEnvMap(float3 startPoint, float3 dir)
{

}

struct ShiftResult
{
    bool IsSuccessful;
    float Jacobian;
    float3 Wi;
}

// Is this eta relative or n?
ShiftResult ShiftHalfVector(ShadingFrame sframeMain, ShadingFrame sframeShifted, float3 wiMain, float3 woMain, float3 woShifted, float etaMain, float etaShifted)
{
    ShiftResult result = (ShiftResult)0;

    // TODO: Can be handled as special case
    if (etaMain == 1.0f || etaShifted == 1.0f)
    {
        result.IsSuccessful = false;
        return result;
    }

    float3 V_s = sframeMain.ToLocal(woMain);
    float3 V_s_shifted = sframeShifted.ToLocal(woShifted);
    float3 L_s = sframeMain.ToLocal(wiMain);

    float NdV = SSpaceCosTheta(V_s);
    float NdL = SSpaceCosTheta(L_s);

    float3 L_s_shifted;

    bool isRefraction = NdV * NdL < 0.0f;

    if (isRefraction)
    {
        float3 H_s = L_s + V_s * etaMain; // TODO: Probably wrong
        H_s = normalize(H_s);

        L_s_shifted = refract(-V_s_shifted, H_s, etaShifted);

        float3 H_s_shifted = L_s_shifted + V_s_shifted * etaShifted; // TODO: Here too
        //H_s_shifted = normalize(H_s_shifted); // ?

        float HLength2 = dot(H_s_shifted, H_s_shifted) / max(EPSILON, dot(H_s, H_s));
        float VdH = abs(dot(V_s, H_s)) / max(EPSILON, abs(dot(V_s_shifted, H_s_shifted)));
        result.Jacobian = HLength2 * VdH;
    }
    else
    {
        float3 H_s = normalize(L_s + V_s);

        float3 N_s = float3(0,0,1);
        L_s_shifted = NormalizeSafe(reflect(-V_s_shifted, H_s), N_s);

        float VdH = abs(dot(V_s_shifted, H_s)) / max(EPSILON, abs(dot(V_s, H_s_shifted)));
        result.Jacobian = abs(VdH);
    }

    result.IsSuccessful = true;
    result.Wi = sframeShifted.ToWorld(L_s_shifted);
    return result;
}

ShiftResult ShiftReconnect(PathVertex mainSource, PathVertex shiftSource, PathVertex dest)
{

}

#endif