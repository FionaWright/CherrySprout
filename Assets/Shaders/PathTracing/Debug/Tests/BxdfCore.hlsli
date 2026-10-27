#ifndef H_TEST_BXDF_CORE_H
#define H_TEST_BXDF_CORE_H

#if FEAT_DBG_D(BxdfTestCore)

#include "Utils/SharedUtils.h"
#include "Utils/Debug/DebugWindow.hlsli"
#include "PathTracing/Debug/Mutex.hlsli"

// TODO: Turn off gamma correct

void Test_BxdfCore(uint2 pixelCoord)
{
    float width = gSettings.FrameDimensions.x;
    float height = gSettings.FrameDimensions.y;

    //DebugWindow w = CreateDebugWindow(float2(0,0), float2(height/width,1));
    DebugWindow w1 = CreateDebugWindow(float2(0,0), float2(0.333,1));
    DebugWindow w2 = CreateDebugWindow(float2(0.333,0), float2(0.666,1));
    DebugWindow w3 = CreateDebugWindow(float2(0.666,0), float2(1,1));
    float2 pixelUV = float2(pixelCoord) / float2(gSettings.FrameDimensions);

    gTexPrimal[pixelCoord].rgb = 0.0f;

    RngInfo rngInfo = InitializeRngInfo(pixelCoord, 0, gSettings.FrameIdx, gSettings.Seed);

    HitInfo hitInfo;
    hitInfo.Mat.Albedo = 1.0f;
    hitInfo.Mat.TransmissionColor = 1.0f;
    hitInfo.Emission = 0.0f;
    hitInfo.Mat.Metallic = 1.0f;
    hitInfo.Mat.TransmissionFactor = 0.0f;
    hitInfo.Mat.IOR_N = 1.5f;
    hitInfo.Ns_ff = float3(0,0,1);

    hitInfo.SFrame = CreateShadingFrame(hitInfo.Ns_ff);
    hitInfo.IsEntering = true;

    float3 wo = normalize(float3(1, 0, 1));

    float threshold = 1e-6f;

    if (w1.Select(pixelUV))
    {
        hitInfo.Mat.Roughness = 0.3f;

        float cosTheta = RemapUtoS(w1.UV.y);
        float phi = w1.UV.x * 2 * PI;
        float3 wi = CylinderToSphere(cosTheta, phi);

        BxdfEval bxdfEval = BxDF::Evaluate(hitInfo, wo, wi, LobeTag::eAll);

        gTexPrimal[pixelCoord].rgb = bxdfEval.PDF > threshold ? bxdfEval.PDF : float3(1,0,0);
    }

    if (w2.Select(pixelUV))
    {
        hitInfo.Mat.Roughness = 0.1f;

        float cosTheta = RemapUtoS(w2.UV.y);
        float phi = w2.UV.x * 2 * PI;
        float3 wi = CylinderToSphere(cosTheta, phi);

        BxdfEval bxdfEval = BxDF::Evaluate(hitInfo, wo, wi, LobeTag::eAll);

        gTexPrimal[pixelCoord].rgb = bxdfEval.PDF > threshold ? bxdfEval.PDF : float3(1,0,0);
    }

    if (w3.Select(pixelUV))
    {
        float cosTheta = RemapUtoS(w3.UV.y);
        float phi = w3.UV.x * 2 * PI;
        float3 wi = CylinderToSphere(cosTheta, phi);

        BxdfEval bxdfEval3;
        BxdfEval bxdfEval1;
        {
            hitInfo.Mat.Roughness = 0.3f;
            bxdfEval3 = BxDF::Evaluate(hitInfo, wo, wi, LobeTag::eAll);
        }
        {
            hitInfo.Mat.Roughness = 0.1f;
            bxdfEval1 = BxDF::Evaluate(hitInfo, wo, wi, LobeTag::eAll);
        }

        bool overlap = (bxdfEval1.PDF <= threshold) == (bxdfEval3.PDF <= threshold);

        gTexPrimal[pixelCoord].rgb = overlap ? float3(0,1,0) : float3(1,0,0);
    }
}

#else

void Test_BxdfCore(uint2 pixelCoord) { }

#endif

#endif