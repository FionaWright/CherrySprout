#ifndef H_SHIFT_MAPPING_H
#define H_SHIFT_MAPPING_H

// https://github.com/gradientpm/gradient-mts/blob/master/src/integrators/gradient/gpt/shift_mapping/shiftmapping.h

void TestVisibilityPoints(float3 source, float3 dest, out bool occluded)
{
    RayQuery<RAY_FLAGS> q;

    float distance = length(dest - source);
    float3 dir = normalize(dest - source);

    RayDesc ray;
    ray.Origin = source;
    ray.Direction = dir;
    ray.TMin = 0.001;
    ray.TMax = distance;

    q.TraceRayInline(gTLAS, RAY_FLAGS, 0xFF, ray);
    q.Proceed();

    occluded = q.CommittedStatus() == COMMITTED_TRIANGLE_HIT;
}

void TestVisibilityDirection(float3 source, float3 dir, out bool occluded)
{
    RayQuery<RAY_FLAGS> q;

    float distance = 1000.0f;

    RayDesc ray;
    ray.Origin = source;
    ray.Direction = dir;
    ray.TMin = 0.001;
    ray.TMax = distance;

    q.TraceRayInline(gTLAS, RAY_FLAGS, 0xFF, ray);
    q.Proceed();

    occluded = q.CommittedStatus() == COMMITTED_TRIANGLE_HIT;
}

struct ShiftResult
{
    bool IsSuccessful;
    float Jacobian;
    float3 Wi;
};

// Is this eta relative or n?
ShiftResult ShiftHalfVector(ShadingFrame sframeMain, ShadingFrame sframeShifted, float3 wiMain, float3 woMain, float3 woShifted, float etaMain, float etaShifted)
{
    ShiftResult result = (ShiftResult)0;

    // TODO: Can be handled as special case
    if (etaMain == 1.0f || etaShifted == 1.0f)
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

    if (isRefraction)
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

        float3 H_s_shifted = H_s; // TODO: Guess, idk

        float VdH = abs(dot(V_s_shifted, H_s)) / max(EPSILON, abs(dot(V_s, H_s_shifted)));
        result.Jacobian = abs(VdH);
    }

    result.IsSuccessful = true;
    result.Wi = sframeShifted.ToWorld(L_s_shifted);
    return result;
}

ShiftResult ShiftReconnect(float3 sourceMain, float3 sourceShifted, float3 normalShifted, float3 dest, float3 normalDest)
{
    ShiftResult result;

    bool occluded;
    TestVisibilityPoints(sourceShifted, dest, occluded);

    if (occluded)
    {
        result.IsSuccessful = false;
        DBG_OUTPUT1(1.0f, GD_RejectionShiftMappingRecOcc);
        return result;
    }

    float3 destToMain = sourceMain - dest;
    float3 destToShifted = sourceShifted - dest;

    float3 destToShiftedDir = normalize(destToShifted);
    float3 shiftedToDestDir = -destToShiftedDir;

    float cosSource = dot(normalShifted, shiftedToDestDir);
    float cosDest   = dot(normalDest, destToShiftedDir);

    result.Wi = normalize(-destToShifted);

    float NdL = dot(normalShifted, result.Wi);
    if (NdL <= 0.0f)
    {
        result.IsSuccessful = false;
        DBG_OUTPUT1(1.0f, GD_RejectionShiftMappingRecNdL);
        return result;
    }

    float length2Main = LengthSquared(destToMain);
    float length2Shifted = LengthSquared(destToShifted);

    float cosThetaMain = dot(destToMain, normalDest) / sqrt(length2Main); // ?
    float cosThetaShifted = dot(-result.Wi, normalDest); // ?

    result.IsSuccessful = true;
    result.Jacobian = abs(cosThetaShifted * length2Main) / max(EPSILON, abs(cosThetaMain * length2Shifted));
    //result.Jacobian = abs(cosDest * length2Main) / max(EPSILON, abs(cosSource * length2Shifted));
    return result;
}

ShiftResult ShiftReconnectEnvironment(float3 sourceShifted, float3 normalShifted, float3 dirMain)
{
    ShiftResult result;

    float NdL = dot(normalShifted, dirMain);
    if (NdL <= 0.0f)
    {
        result.IsSuccessful = false;
        DBG_OUTPUT1(1.0f, GD_RejectionShiftMappingEnvNdL);
        return result;
    }

    bool occluded;
    TestVisibilityDirection(sourceShifted, dirMain, occluded);

    if (occluded)
    {
        result.IsSuccessful = false;
        DBG_OUTPUT1(1.0f, GD_RejectionShiftMappingEnvOcc);
        return result;
    }

    result.IsSuccessful = true;
    result.Jacobian = 1.0f;
    result.Wi = dirMain;
    return result;
}

#endif