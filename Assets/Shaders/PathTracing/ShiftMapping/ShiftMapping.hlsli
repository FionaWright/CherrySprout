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

struct ShiftMappingResult
{
    bool IsSuccessful;
    float Jacobian;
    float3 Wo;
}

// Is this eta relative or n?
ShiftMappingResult ShiftHalfVector(float3 wiMain, float3 woMain, float3 wiShifted, float etaMain, float etaShifted)
{
    
}

ShiftMappingResult ShiftReconnect(PathVertex mainSource, PathVertex shiftSource, PathVertex dest)
{

}

#endif