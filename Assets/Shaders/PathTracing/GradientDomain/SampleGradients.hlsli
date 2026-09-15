#ifndef H_SAMPLE_GRADIENT_H
#define H_SAMPLE_GRADIENT_H

#if FEATURE_ENABLED_PP(GradientDomain)

#include "PathTracing/3_GetPrimaryRay.hlsli"
#include "PathTracing/GradientDomain/TraceShiftedRay.hlsli"
#include "PathTracing/MIS.hlsli"

float3 SampleGradients(RngInfo rngInfo, uint2 mainCoord, PathSample mainPath)
{
    float3 gradientSum = float3(0,0,0);

    for (uint j = 0; j < gSettings.GradientNumSamples; j++)
	{
		// TODO: Pick a neighbour
        uint2 shiftedCoord = mainCoord + uint2(1,0);

        float3 shiftedRayOrigin;
        float3 shiftedRayDirection;

        GetPrimaryRay(
            rngInfo, gSettings.CameraPositionWorld, gSettings.TexelSize,
            shiftedCoord, gSettings.InvV, gSettings.InvP,
            gSettings.DofFocalDist, gSettings.DofLensRadius,
            shiftedRayOrigin, shiftedRayDirection);

        float3 Lo;
        float pdf;
        bool isSymmetric;
        TraceShiftedPath(
            mainPath.VertexList,
            shiftedRayOrigin,
            shiftedRayDirection,
            rngInfo,
            shiftedCoord,
            Lo,
            pdf,
            isSymmetric);

        float m = 1.0f;
        if (isSymmetric)
        {
            m = BalanceHeuristic(mainPath.PDF, pdf, 1, gSettings.GradientNumSamples); // ?
        }
        Lo *= m;

        gradientSum += mainPath.Lo - Lo;
	}

    return gradientSum;
}

#else

float3 SampleGradients(RngInfo rngInfo, uint2 mainCoord, PathSample mainPath) { return NAN; }

#endif

#endif