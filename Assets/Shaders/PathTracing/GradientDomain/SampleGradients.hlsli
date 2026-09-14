#ifndef H_SAMPLE_GRADIENT_H
#define H_SAMPLE_GRADIENT_H

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

        bool isSymmetric;
        PathSample shiftedSample = TraceShiftedRay(
                                        mainPath.VertexList,
                                        shiftedRayOrigin,
                                        shiftedRayDirection,
                                        rngInfo,
                                        shiftedCoord,
                                        isSymmetric);

        float m = 1.0f;
        if (isSymmetric)
        {
            m = BalanceHeuristic(mainPath.PDF, shiftedSample.PDF, 1, gSettings.GradientNumSamples); // ?
        }
        shiftedSample.Lo *= m;

        gradientSum += mainPath.Lo - shiftedSample.Lo;
	}

    return gradientSum;
}

#endif