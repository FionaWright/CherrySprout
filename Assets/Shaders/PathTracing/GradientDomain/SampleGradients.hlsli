#ifndef H_SAMPLE_GRADIENT_H
#define H_SAMPLE_GRADIENT_H

#if FEATURE_ENABLED_PP(GradientDomain)

#include "PathTracing/3_GetPrimaryRay.hlsli"
#include "PathTracing/GradientDomain/TraceShiftedPath.hlsli"
#include "PathTracing/MIS.hlsli"

float3 SampleGradients(RngInfo rngInfo, uint2 mainCoord, PathSample mainPath)
{
    float3 gradientSum = float3(0,0,0);

#define MAX_GRADIENT_NUM_SAMPLES 4
    const uint2 OFFSETS[MAX_GRADIENT_NUM_SAMPLES] = {
        uint2(1, 0),
        uint2(0, 1),
        uint2(-1, 0),
        uint2(0, -1)
    };

    for (uint j = 0; j < min(MAX_GRADIENT_NUM_SAMPLES, gSettings.GradientNumSamples); j++)
	{
        DBG_SET_CURRENT_RAY_DEPTH(0);

        uint2 shiftedCoord = mainCoord + OFFSETS[j];

        float3 shiftedRayOrigin;
        float3 shiftedRayDirection;

        GetPrimaryRay(
            rngInfo, gSettings.CameraPositionWorld, gSettings.TexelSize,
            shiftedCoord, gSettings.InvV, gSettings.InvP,
            gSettings.DofFocalDist, gSettings.DofLensRadius,
            shiftedRayOrigin, shiftedRayDirection);

		// TODO: Primary Ray PDF ratio
		float pdfPrimaryMain = 1.0f;
		float pdfPrimaryShifted = 1.0f;
		float pdfRatio = (pdfPrimaryShifted / pdfPrimaryMain);

        DBG_OUTPUT1(pdfRatio, GD_PrimaryPdfRatio);

        float3 Lo;
        bool isSymmetric;
        TraceShiftedPath(
            mainPath.VertexList,
            shiftedRayOrigin,
            shiftedRayDirection,
            rngInfo,
            shiftedCoord,
            Lo,
            pdfRatio,
            isSymmetric);

        // TODO: Lo also multiplied by jacobian?

        float m = 1.0f;
        if (isSymmetric)
        {
            m = BalanceHeuristicRatio(pdfRatio, 1, gSettings.GradientNumSamples);
        }

        // TODO: Split into X and Y
        float3 gradient = mainPath.Lo - Lo;
        gradient *= m;
        gradientSum += gradient;

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
        DBG_OUTPUT3(0.0f, GD_Wi);
        DBG_OUTPUT3(1.0f, GD_Jacobian);
	}

    return gradientSum;
}

#else

float3 SampleGradients(RngInfo rngInfo, uint2 mainCoord, PathSample mainPath) { return NAN; }

#endif

#endif