#ifndef H_SAMPLE_GRADIENT_H
#define H_SAMPLE_GRADIENT_H

#if FEATURE_ENABLED_PP(GradientDomain)

#include "PathTracing/3_GetPrimaryRay.hlsli"
#include "PathTracing/GradientDomain/TraceShiftedPath.hlsli"
#include "PathTracing/MIS.hlsli"

float3 SampleGradient(RngInfo rngInfo, uint2 shiftedCoord, PathSample mainPath)
{
    DBG_SET_CURRENT_RAY_DEPTH(0);

    float3 shiftedRayOrigin;
    float3 shiftedRayDirection;

    GetPrimaryRay(
        rngInfo, gSettings.CameraPositionWorld, gSettings.TexelSize,
        shiftedCoord, gSettings.InvV, gSettings.InvP,
        gSettings.DofFocalDist, gSettings.DofLensRadius,
        shiftedRayOrigin, shiftedRayDirection);

	float pdfRatio = 1.0f;
    DBG_OUTPUT1(pdfRatio, GD_PrimaryPdfRatio);

    float3 Lo;
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

    float m = 1.0f;
    if (isSymmetric)
    {
        m = BalanceHeuristicRatio(pdfRatio, 1, 2); // TODO: 2 or 1?
    }

    DBG_OUTPUT1(pdfRatio, GD_PdfRatio);
    DBG_OUTPUT1(isSymmetric, GD_IsSymmetric);
    DBG_OUTPUT1(m, GD_MIS);
    DBG_OUTPUT3(Lo, GD_Lo);
    DBG_OUTPUT1(0.0f, GD_RejectionUninit);
    DBG_OUTPUT1(0.0f, GD_RejectionMaxVertex);
    DBG_OUTPUT1(0.0f, GD_RejectionEnvMap);
    DBG_OUTPUT1(0.0f, GD_RejectionShiftMapping);
    DBG_OUTPUT1(0.0f, GD_RejectionVertexMismatch);
    DBG_OUTPUT3(0.0f, GD_FinalReconnectionState);
    DBG_OUTPUT3(0.0f, GD_V2Type);
    DBG_OUTPUT1(0.0f, GD_PdfShifted);
    DBG_OUTPUT3(0.0f, GD_L_w);
    DBG_OUTPUT3(0.0f, GD_L_s);
    DBG_OUTPUT3(1.0f, GD_Jacobian);

    float3 gradient = mainPath.Lo - Lo;
    gradient *= m;
    return gradient;
}

void SampleGradients(RngInfo rngInfo, uint2 mainCoord, PathSample mainPath, out float3 gradientX, out float3 gradientY)
{
    // TODO: More samples?
    gradientX = SampleGradient(rngInfo, mainCoord + uint2(1,0), mainPath);
    gradientY = SampleGradient(rngInfo, mainCoord + uint2(0,1), mainPath);
}

#else

void SampleGradients(RngInfo rngInfo, uint2 mainCoord, PathSample mainPath, out float3 gradientX, out float3 gradientY) { gradientX = NAN; gradientY = NAN; }

#endif

#endif