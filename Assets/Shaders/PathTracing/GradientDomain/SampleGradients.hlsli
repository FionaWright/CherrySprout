#ifndef H_SAMPLE_GRADIENT_H
#define H_SAMPLE_GRADIENT_H

#if FEAT_CORE_D(GradientDomain)

#include "PathTracing/3_GetPrimaryRay.hlsli"
#include "PathTracing/Shift/TraceShiftedPath.hlsli"
#include "PathTracing/MIS.hlsli"

void EvaluateShiftedPath(int2 shiftedCoord, PathSample mainPath, out float3 gradient)
{
    gradient = 0.0f;
    if (shiftedCoord.x < 0 ||
        shiftedCoord.y < 0 ||
        shiftedCoord.x >= gSettings.FrameDimensions.x ||
        shiftedCoord.y >= gSettings.FrameDimensions.y)
        return;

    DBG_SET_CURRENT_RAY_DEPTH(0);

    float3 shiftedRayOrigin;
    float3 shiftedRayDirection;

    GetPrimaryRay(
        mainPath.RngPrimary, gSettings.CameraPositionWorld, gSettings.TexelSize,
        shiftedCoord, gSettings.InvV, gSettings.InvP,
        gSettings.DofFocalDist, gSettings.DofLensRadius,
        shiftedRayOrigin, shiftedRayDirection);

	float pdfRatioPrimary = 1.0f;
    DBG_OUTPUT1(pdfRatioPrimary, GD_PrimaryPdfRatio);

    float3 shiftedLo;
    float3 subpathGradients;
    float jacobian;
    float pdfRatio;
    bool isSymmetric;
    TraceShiftedPath(
        mainPath.VertexList,
        shiftedRayOrigin,
        shiftedRayDirection,
        shiftedCoord,
        shiftedLo,
        subpathGradients,
        jacobian,
        pdfRatio,
        isSymmetric);

    if (isSymmetric)
    {
        shiftedLo *= jacobian;
        pdfRatio *= jacobian;

        pdfRatio *= pdfRatioPrimary;

        float mis_path = 1.0f;
        if (isSymmetric)
        {
            if (FEAT_DBG(FixedMisHalf))
                mis_path = 0.5f;
            else
                mis_path = BalanceHeuristicRatio(pdfRatio, 1, 1);
        }

        gradient = (shiftedLo - mainPath.Lo) * mis_path;
        gradient += subpathGradients;

        DBG_OUTPUT1(pdfRatio, GD_PdfRatio);
        DBG_OUTPUT1(isSymmetric, GD_IsSymmetric);
        DBG_OUTPUT1(mis_path, GD_MIS);
        DBG_OUTPUT3(shiftedLo, GD_Lo);
    }

    // Debug
    {
        DBG_OUTPUT_DUMP_DISABLE();
        DBG_OUTPUT1(0.0f, GD_RejectionUninit);
        DBG_OUTPUT1(0.0f, GD_RejectionDirac);
        DBG_OUTPUT1(0.0f, GD_RejectionMaxVertex);
        DBG_OUTPUT1(0.0f, GD_RejectionEnvMap);
        DBG_OUTPUT1(0.0f, GD_RejectionEmission);
        DBG_OUTPUT1(0.0f, GD_RejectionShiftMapping);
        DBG_OUTPUT1(0.0f, GD_RejectionNEE);
        DBG_OUTPUT1(0.0f, GD_RejectionShiftMappingHalfVec);
        DBG_OUTPUT1(0.0f, GD_RejectionShiftMappingRecOcc);
        DBG_OUTPUT1(0.0f, GD_RejectionShiftMappingRecNdL);
        DBG_OUTPUT1(0.0f, GD_RejectionShiftMappingEnvOcc);
        DBG_OUTPUT1(0.0f, GD_RejectionShiftMappingEnvNdL);
        DBG_OUTPUT1(0.0f, GD_FinalReconnectionState);
        DBG_OUTPUT1(0.0f, GD_V2Type);
        DBG_OUTPUT1(0.0f, GD_V2Lobe);
        DBG_OUTPUT1(0.0f, GD_PdfShifted);
        DBG_OUTPUT1(0.0f, GD_L_w);
        DBG_OUTPUT1(0.0f, GD_L_s);
        DBG_OUTPUT1(0.0f, GD_IsLobeDirac);
        DBG_OUTPUT1(0.0f, GD_NeeContrib);
        DBG_OUTPUT1(1.0f, GD_Jacobian);
        DBG_OUTPUT_DUMP_ENABLE();
    }
}

Gradients SampleGradients(int2 mainCoord, PathSample mainPath)
{
    // XF, XB, YF, YB
    int2 offsets[4] = {
        int2(1,0),
        int2(-1,0),
        int2(0,1),
        int2(0,-1)
    };

    float3 grad[4];

    [loop] // Note: Do not unroll, otherwise expensive inlining will occur
    for (int i = 0; i < 4; i++)
    {
        if (i == 1)
            { DBG_OUTPUT_ENABLE(); }
        else
            { DBG_OUTPUT_DISABLE(); }

        EvaluateShiftedPath(mainCoord + offsets[i], mainPath, grad[i]);
    }
    DBG_OUTPUT_ENABLE()

    Gradients gradients;
    gradients.XForward = grad[0];
    gradients.XBackward = -grad[1];

    gradients.YForward = grad[2];
    gradients.YBackward = -grad[3];
    return gradients;
}

#else

Gradients SampleGradients(int2 mainCoord, PathSample mainPath) { return (Gradients)0; }

#endif

#endif