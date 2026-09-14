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
ShiftResult ShiftHalfVector(ShadingFrame sframe, float3 wiMain, float3 woMain, float3 woShifted, float iorNMain, float iorNShifted)
{
    ShiftResult result;

    float3 V_s = sframe.ToLocal(woMain);
    float3 L_s = sframe.ToLocal(wiMain);

    float NdV = SSpaceCosTheta(V_s);
    float NdL = SSpaceCosTheta(L_s);

    float3 L_s_shifted;

    bool isRefraction = NdV * NdL < 0.0f;

    if (isRefraction)
    {
        float3 H_s = NdL < 0
        L_s_shifted =
    }
}

ShiftResult ShiftReconnect(PathVertex mainSource, PathVertex shiftSource, PathVertex dest)
{

}

#endif