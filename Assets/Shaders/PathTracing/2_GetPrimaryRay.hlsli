#ifndef H_GET_PRIMARY_RAY_H
#define H_GET_PRIMARY_RAY_H

#if (DEBUG_ENABLED(BxdfTestRevaluate) || DEBUG_ENABLED(BxdfTestHemisphere))
static bool gRunBxdfTestForPixel = false;
#endif

void GetPrimaryRay(
    inout RngInfo rngInfo,
    float3 cameraPos,
    float2 texelSize;
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

    float2 pixelUV = pixelCoord;
    if (FEATURE_ENABLED(Jitter))
    {
        float rJitterX = Rand01(rngInfo);
        float rJitterY = Rand01(rngInfo);
        float2 jitter = float2(rJitterX, rJitterY) - 0.5f;
        pixelUV += jitter;
    }
    pixelUV *= texelSize;

    if ((DEBUG_ENABLED(BxdfTestRevaluate) || DEBUG_ENABLED(BxdfTestHemisphere)) && pixelUV.x < 0.5f)
    {
        pixelUV.x = 1 - pixelUV.x;
        gRunBxdfTestForPixel = true;
    }

    float2 ndc = RemapUtoS(pixelUV);
    ndc.y = -ndc.y;
    float4 clip = float4(ndc, 0, 1); // z=0 for near plane
    float4 view = mul(invP, clip);
    view /= view.w;
    float4 world = mul(invV, view);
    direction = normalize(world.xyz - origin);

    if (!FEATURE_ENABLED(DepthOfField))
        return;

    float3 camRight = normalize(float3(invV[0][0], invV[1][0], invV[2][0]));
    float3 camUp = normalize(float3(invV[0][1], invV[1][1], invV[2][1]));
    float3 focalPoint = origin + direction * dofFocalDist;

    float rLensU = Rand01(rngInfo);
    float rLensV = Rand01(rngInfo);
    float r = sqrt(rLensU) * dofLensRadius;
    float theta = 2.0 * PI * rLensV;
    float3 lensOffset = r * (camRight * cos(theta) + camUp * sin(theta));

    origin += lensOffset;
    direction = normalize(focalPoint - origin);
}

#endif