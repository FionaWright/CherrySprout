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

    DebugWindow w = CreateDebugWindow(float2(0,0), float2(height/width,1));
    //DebugWindow w = CreateDebugWindow(float2(0,0), float2(1,1));
    float2 pixelUV = float2(pixelCoord) / float2(gSettings.FrameDimensions);

    gTexPrimal[pixelCoord].rgb = 0.0f;

    if (w.Select(pixelUV))
    {
        RngInfo rngInfo = InitializeRngInfo(pixelCoord, 0, gSettings.FrameIdx, gSettings.Seed);

        HitInfo hitInfo;
        hitInfo.Mat.Albedo = 1.0f;
        hitInfo.Mat.TransmissionColor = 1.0f;
        hitInfo.Emission = 0.0f;
        hitInfo.Mat.Roughness = 0.5f;
        hitInfo.Mat.Metallic = 1.0f;
        hitInfo.Mat.TransmissionFactor = 1.0f;
        hitInfo.Mat.IOR_N = 1.5f;
        hitInfo.Ns_ff = float3(0,0,1);
        hitInfo.SFrame = CreateShadingFrame(hitInfo.Ns_ff);
        hitInfo.IsEntering = true;

        float3 wo = normalize(float3(1, 0, 1));

        BxDF bxdf;

        DebugWindow wConstant = CreateDebugWindow(float2(0,0), float2(0.5,1));
        DebugWindow wRand = CreateDebugWindow(float2(0.5,0), float2(1,1));
        if (wConstant.Select(w.UV))
        {
            float cosTheta = RemapUtoS(wConstant.UV.y);
            float phi = wConstant.UV.x * 2 * PI;
            float3 wi = CylinderToSphere(cosTheta, phi);

            BxdfEval bxdfEval = bxdf.Evaluate(hitInfo, wo, wi, LobeTag::eAll);

            //gTexPrimal[pixelCoord].rgb = wi;
            gTexPrimal[pixelCoord].rgb = bxdfEval.PDF;
        }
        else if (false && wRand.Select(w.UV))
        {
            BxdfEval bxdfEval;
            BxdfSample bxdfSample = bxdf.Sample(rngInfo, hitInfo, wo, bxdfEval);
            float3 wi = bxdfSample.Wi;

            float u1 = Rand01(rngInfo);
            float u2 = Rand01(rngInfo);
            wi = RandHemisphereUniformSSpace(u1, u2);

            float2 cy = SphereToCylinder(wi);
            float2 uv = float2(
                cy.y / (2.0 * PI),
                cy.x
            );

            uv = float2(Rand01(rngInfo), Rand01(rngInfo));

            uint2 realCoord = w.Invert(wRand.Invert(uv)) * gSettings.FrameDimensions;
            //uint2 realCoord = pixelCoord;
            //uint2 realCoord = w.Invert(wRand.Invert(wRand.UV)) * gSettings.FrameDimensions;

            uint mutexId = realCoord.x + realCoord.y * gSettings.FrameDimensions.x;
            if (TryAcquireSpinLockPixel(mutexId))
            {
                //gTexPrimal[realCoord].rgb = bxdfEval.PDF;
                //gTexPrimal[realCoord].rgb = float3(wRand.Invert(wRand.UV), 0);
                gTexPrimal[realCoord].rgb = float3(uv, 0);
                ReleaseMutexPixel(mutexId);
            }
        }
    }
}

#else

void Test_BxdfCore(uint2 pixelCoord) { }

#endif

#endif