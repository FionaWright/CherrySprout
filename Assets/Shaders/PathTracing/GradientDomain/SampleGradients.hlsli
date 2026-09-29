#ifndef H_SAMPLE_GRADIENT_H
#define H_SAMPLE_GRADIENT_H

#if FEATURE_ENABLED_PP(GradientDomain)

#include "PathTracing/3_GetPrimaryRay.hlsli"
#include "PathTracing/GradientDomain/TraceShiftedPath.hlsli"
#include "PathTracing/MIS.hlsli"

void sampleShiftedPath(RngInfo rngInfo, int2 shiftedCoord, PathSample mainPath, out float3 Lo, out float misWeight)
{
    if (shiftedCoord.x < 0 ||
        shiftedCoord.y < 0 ||
        shiftedCoord.x >= gSettings.FrameDimensions.x ||
        shiftedCoord.y >= gSettings.FrameDimensions.y)
    {
        Lo = mainPath.Lo;
        misWeight = 1.0f;
        return;
    }

    DBG_SET_CURRENT_RAY_DEPTH(0);

    float3 shiftedRayOrigin;
    float3 shiftedRayDirection;

    GetPrimaryRay(
        rngInfo, gSettings.CameraPositionWorld, gSettings.TexelSize,
        shiftedCoord, gSettings.InvV, gSettings.InvP,
        gSettings.DofFocalDist, gSettings.DofLensRadius,
        shiftedRayOrigin, shiftedRayDirection);

	float pdfRatio = 1.0f; // Shifted / Main
    DBG_OUTPUT1(pdfRatio, GD_PrimaryPdfRatio);

    bool isSymmetric;
    TraceShiftedPath(
        mainPath.VertexList,
        shiftedRayOrigin,
        shiftedRayDirection,
        shiftedCoord,
        Lo,
        pdfRatio,
        isSymmetric);

    misWeight = 1.0f;
    if (isSymmetric)
    {
        // Two estimators, forward + backward
        misWeight = BalanceHeuristicRatio(pdfRatio, 1, 1);
    }

    // Debug
    {
        DBG_OUTPUT1(pdfRatio, GD_PdfRatio);
        DBG_OUTPUT1(isSymmetric, GD_IsSymmetric);
        DBG_OUTPUT1(misWeight, GD_MIS);
        DBG_OUTPUT3(Lo, GD_Lo);
        DBG_OUTPUT1(0.0f, GD_RejectionUninit);
        DBG_OUTPUT1(0.0f, GD_RejectionMaxVertex);
        DBG_OUTPUT1(0.0f, GD_RejectionEnvMap);
        DBG_OUTPUT1(0.0f, GD_RejectionShiftMapping);
        DBG_OUTPUT1(0.0f, GD_RejectionShiftMappingHalfVec);
        DBG_OUTPUT1(0.0f, GD_RejectionShiftMappingRecOcc);
        DBG_OUTPUT1(0.0f, GD_RejectionShiftMappingRecNdL);
        DBG_OUTPUT1(0.0f, GD_RejectionShiftMappingEnvOcc);
        DBG_OUTPUT1(0.0f, GD_RejectionShiftMappingEnvNdL);
        DBG_OUTPUT1(0.0f, GD_RejectionVertexMismatch);
        DBG_OUTPUT1(0.0f, GD_FinalReconnectionState);
        DBG_OUTPUT1(0.0f, GD_V2Type);
        DBG_OUTPUT1(0.0f, GD_PdfShifted);
        DBG_OUTPUT1(0.0f, GD_L_w);
        DBG_OUTPUT1(0.0f, GD_L_s);
        DBG_OUTPUT1(1.0f, GD_Jacobian);
    }
}

Gradients SampleGradients(RngInfo rngInfo, int2 mainCoord, PathSample mainPath)
{
    // XF, XB, YF, YB
    int2 offsets[4] = {
        int2(1,0),
        int2(-1,0),
        int2(0,1),
        int2(0,-1)
    };

    float3 Lo[4];
    float mis[4];

    [loop] // Note: Do not unroll, otherwise expensive inlining will occur
    for (int i = 0; i < 4; i++)
    {
        sampleShiftedPath(rngInfo, mainCoord + offsets[i], mainPath, Lo[i], mis[i]);
    }

    Gradients gradients;
    gradients.XForward = ((Lo[0] - mainPath.Lo) * mis[0]);
    gradients.XBackward = ((mainPath.Lo - Lo[1]) * mis[1]);

    gradients.YForward = ((Lo[2] - mainPath.Lo) * mis[2]);
    gradients.YBackward = ((mainPath.Lo - Lo[3]) * mis[3]);
    return gradients;
}

#else

Gradients SampleGradients(RngInfo rngInfo, int2 mainCoord, PathSample mainPath) { return (Gradients)0; }

#endif

#endif