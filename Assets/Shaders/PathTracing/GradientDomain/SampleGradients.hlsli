#ifndef H_SAMPLE_GRADIENT_H
#define H_SAMPLE_GRADIENT_H

#if FEATURE_ENABLED_PP(GradientDomain)

#include "PathTracing/3_GetPrimaryRay.hlsli"
#include "PathTracing/GradientDomain/TraceShiftedPath.hlsli"
#include "PathTracing/MIS.hlsli"

void sampleShiftedPath(RngInfo rngInfo, uint2 shiftedCoord, PathSample mainPath, out float3 Lo, out float misWeight)
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

    // TODO: Lo also multiplied by jacobian?

    misWeight = 1.0f;
    if (isSymmetric)
    {
        misWeight = BalanceHeuristicRatio(pdfRatio, 2, 1);
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

Gradients SampleGradients(RngInfo rngInfo, uint2 mainCoord, PathSample mainPath)
{
    float3 LoXForward;
    float misXForward;
    sampleShiftedPath(rngInfo, mainCoord + uint2(1,0), mainPath, LoXForward, misXForward);

    float3 LoXBackward;
    float misXBackward;
    sampleShiftedPath(rngInfo, mainCoord + uint2(-1,0), mainPath, LoXBackward, misXBackward);

    float3 LoYForward;
    float misYForward;
    sampleShiftedPath(rngInfo, mainCoord + uint2(0,1), mainPath, LoYForward, misYForward);

    float3 LoYBackward;
    float misYBackward;
    sampleShiftedPath(rngInfo, mainCoord + uint2(0,-1), mainPath, LoYBackward, misYBackward);

    Gradients gradients;
    gradients.XForward = ((LoXForward - mainPath.Lo) * misXForward);
    gradients.XBackward = ((mainPath.Lo - LoXBackward) * misXBackward);
    gradients.YForward = ((LoYForward - mainPath.Lo) * misYForward);
    gradients.YBackward = ((mainPath.Lo - LoYBackward) * misYBackward);

    return gradients;
}

#else

void SampleGradients(RngInfo rngInfo, uint2 mainCoord, PathSample mainPath, out float3 gradientX, out float3 gradientY) { gradientX = NAN; gradientY = NAN; }

#endif

#endif