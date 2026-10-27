#ifndef H_GET_PRIMARY_RAY_H
#define H_GET_PRIMARY_RAY_H

#include "PathTracing/RandomSample.hlsli"
#include "PathTracing/FeatFlags/FeatFlags.hlsli"
#include "PathTracing/Debug/Globals.hlsli"
#include "PathTracing/Debug/Internal/OutputColorMacros.hlsli"
#include "Utils/HlslUtils.hlsli"

void GetPrimaryRay(
    RngValuesPrimary rng,
    float3 cameraPos,
    float2 texelSize,
    uint2 pixelCoord,

    float4x4 invV,
    float4x4 invP,

    float dofFocalDist,
    float dofLensRadius,

    out float3 origin,
    out float3 direction
)
{
    origin = cameraPos;

    float2 pixelUV = pixelCoord + 0.5f;

    float2 jitter = 0.0f;
    if (FEAT_CORE(JitterPerPixel))
        jitter = rng.Jitter - 0.5f;
    else if (FEAT_CORE(JitterPerFrame))
        jitter = gSettings.JitterPerFrame;
    pixelUV += jitter;

    DBG_OUTPUT2(jitter, Jitter);

    pixelUV *= texelSize;

    if ((FEAT_DBG(BxdfTestRevaluate) || FEAT_DBG(BxdfTestHemisphere) || FEAT_DBG(BxdfTestLobe)))
    {
        bool runTest = true;
        if (!FEAT_DBG(BxdfTestFullMode))
        {
            if (pixelUV.x < 0.5f)
                pixelUV.x = 1 - pixelUV.x;
            else
                runTest = false;
        }

        if (runTest)
            gRunBxdfTestForPixel = true;
    }

    float2 ndc = RemapUtoS(pixelUV);
    ndc.y = -ndc.y;
    float4 clip = float4(ndc, 0, 1); // z=0 for near plane
    float4 view = mul(invP, clip);
    view /= view.w;
    float4 world = mul(invV, view);
    direction = normalize(world.xyz - origin);

    if (!FEAT_CORE(DepthOfField))
        return;

    float3 camRight = normalize(float3(invV[0][0], invV[1][0], invV[2][0]));
    float3 camUp = normalize(float3(invV[0][1], invV[1][1], invV[2][1]));
    float3 focalPoint = origin + direction * dofFocalDist;

    float r = sqrt(rng.DoF.x) * dofLensRadius;
    float theta = 2.0 * PI * rng.DoF.y;
    float3 lensOffset = r * (camRight * cos(theta) + camUp * sin(theta));

    origin += lensOffset;
    direction = normalize(focalPoint - origin);
}

#endif