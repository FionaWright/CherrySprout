#ifndef H_SHIFT_MAPPING_H
#define H_SHIFT_MAPPING_H

// https://github.com/gradientpm/gradient-mts/blob/master/src/integrators/gradient/gpt/shift_mapping/shiftmapping.h

void TestVisibilityPoints(float3 source, float3 dest, out bool occluded)
{
    RayQuery<RAY_FLAGS> q;

    float3 distance = length(dest - source);
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

    float3 distance = 1000.0f;

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

        float HLength2 = LengthSquared(H_s_shifted) / max(EPSILON, LengthSquared(H_s));
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

ShiftResult ShiftReconnect(float3 sourceMain, float3 sourceShifted, float3 dest, float3 destNormal)
{
    ShiftResult result;

    bool occluded;
    TestVisibilityPoints(sourceShifted, dest, occluded);

    if (occluded)
    {
        result.IsSuccessful = false;
        return result;
    }

    float3 edgeMain = sourceMain - dest;
    float3 edgeShifted = sourceShifted - dest;

    result.Wi = normalize(-edgeShifted);

    float length2Main = LengthSquared(edgeMain);
    float length2Shifted = LengthSquared(edgeShifted);

    float cosThetaMain = dot(edgeMain, destNormal) / sqrt(length2Main); // ?
    float cosThetaShifted = dot(result.Wi, destNormal); // ?

    result.IsSuccessful = true;
    result.Jacobian = abs(cosThetaShifted, length2Main) / max(EPSILON, abs(cosThetaMain * length2Shifted));
    return result;
}

ShiftResult ShiftReconnectEnvironment(float3 sourceMain, float3 dirMain)
{
    ShiftResult result;

    bool occluded;
    TestVisibilityDirection(sourceMain, dirMain, occluded);

    if (occluded)
    {
        result.IsSuccessful = false;
        return result;
    }

    result.IsSuccessful = true;
    result.Jacobian = 1.0f;
    result.Wi = dirMain;
    return result;
}

#endif