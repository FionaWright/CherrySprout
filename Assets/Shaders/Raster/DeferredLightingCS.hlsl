#include "Utils/CBVs.h"
#include "Utils/Math/Punctual.hlsli"
#include "MicrofacetModels/MicrofacetUtils.hlsli"
#include "Utils/SharedUtils.h"
#include "Utils/Math/ShadingFrame.h"
#include "Scene/Material.h"
#include "Utils/Debug/Palette.h"
#include "Utils/Random.h"
#include "PostProcessing/ToneMapping.hlsli"

#include "Raster/Utils/ShadowMapPuncInfo.h"

#include "PathTracing/Structs.h"

#define BXDF_MODE BXDF_PBR
//#define BXDF_MODE BXDF_LAMBERTIAN
#include "BxDFs/GetBxDF.hlsli"

ConstantBuffer<CbvForward> gSettings                          : register(b0, space0);
ConstantBuffer<CbvShadowMapSample> gSettingsShadows           : register(b1, space0);

StructuredBuffer<Material> gMaterials                         : register(t0, space0);
StructuredBuffer<PunctualLight> gPunctuals                    : register(t1, space0);
StructuredBuffer<ShadowMapPuncInfo> gPunctualShadowMapInfo    : register(t2, space0);

Texture2D<float2> gBrdfInt                                    : register(t3, space0);
TextureCube gSkyBox                                           : register(t4, space0);
Texture2D<float> gShadowMapDir                                : register(t5, space0);

Texture2D<uint> gGBufferMaterialIdx                           : register(t6, space0);
Texture2D<float4> gGBufferNormalsUv                           : register(t7, space0);
Texture2D<float> gGBufferDepth                                : register(t8, space0);
Texture2D<float4> gGBufferMv                                  : register(t9, space0);
Texture2D<float4> gTexBloom                                   : register(t10, space0);

Texture2D<float4> gSceneTextures[]                            : register(t0, space1);
TextureCube<float> gShadowMapsPoint[]                         : register(t0, space2);
Texture2D<float> gShadowMapsDistant[]                         : register(t0, space3);

RWTexture2D<float4> gTexPrimal                                : register(u0, space0);

SamplerState gSamplerLinearWrap                               : register(s0, space0);
SamplerState gSamplerPointClamp                               : register(s1, space0);

#include "PathTracing/HitInfo/ApplyMaterialTextures.hlsli"
#include "PathTracing/HitInfo/ReconstructPrimaryRay.hlsli"
#include "PathTracing/3_GetPrimaryRay.hlsli"
#include "Raster/Utils/EvaluatePunctuals.hlsli"
#include "Raster/Utils/Shadows.hlsli"

[numthreads(16, 16, 1)]
void CSMain(uint3 DTid : SV_DispatchThreadID)
{
    uint2 pixelCoord = DTid.xy;

    HitInfo hitInfo;
    bool isMiss;
    float depthNonLinear;
    ReconstructPrimaryRayHit(
        gSettings.CameraPosition,
        pixelCoord,
        gGBufferMaterialIdx,
        gGBufferNormalsUv,
        gGBufferDepth,
        gMaterials,
        hitInfo,
        isMiss,
        depthNonLinear);

    if (isMiss)
    {
        float3 origin;
        float3 wi;

        RngValuesPrimary rng = (RngValuesPrimary)0;
        GetPrimaryRay(
            rng, gSettings.CameraPosition, gSettings.TexelSize, pixelCoord,
            gSettings.InvV, gSettings.InvP, 0.0f, 0.0f,
            origin, wi);

        float3 envSample = gSkyBox.SampleLevel(gSamplerLinearWrap, wi, 0).rgb;
        gTexPrimal[pixelCoord] = float4(envSample, 1.0f);
        return;
    }

    float3 wo = normalize(gSettings.CameraPosition - hitInfo.HitPos.xyz);

    float NdV = saturate(dot(hitInfo.Ns_ff, wo));

    //float3 irradianceIblSample = gIrradiance.SampleLevel(gSamplerLinearWrap, Ns, 0).rgb;
    //float3 diffuseIBL = f_bxdf * irradianceIblSample;

    RngInfo rngInfo = InitializeRngInfo(pixelCoord, 0, gSettings.FrameIdx, 12005);

    float3 E_direct = 0.0f;
    if (ForwardFlagGet(gSettings.Flags, ForwardFlags::eDirectionalLight))
    {
        float3 dirLightRadiance = 1.0f;

        float3 wi = -gSettings.DirLightDir;
        float NdL = max(1e-6f, dot(hitInfo.Ns_ff, wi));
        if (NdL > 0.0f)
        {
            BxdfEval bxdfEval = BxDF::Evaluate(hitInfo, wo, wi, LobeTag::eAll);

            float shadowFactor = 1.0f;
            if (ForwardFlagGet(gSettings.Flags, ForwardFlags::eShadowMaps))
            {
                uint cascadeIdx = 0; // TODO

                float3 normalBias = hitInfo.Ns_ff * gSettingsShadows.NormalBiasDistant;
                float4 biasedPos = float4(hitInfo.HitPos.xyz + normalBias, 1); // TODO: w = 1 ?

                float4 shadowPos = mul(gSettingsShadows.ShadowMatrices[cascadeIdx], biasedPos);
                shadowPos.xyz = shadowPos.xyz / shadowPos.w;
                shadowPos.xy = RemapStoU(shadowPos.xy);

                shadowFactor = GetShadowFactorDirectional(rngInfo, shadowPos[cascadeIdx]);
            }

            if (bxdfEval.PDF > 0.0f)
                E_direct += shadowFactor * NdL * dirLightRadiance * bxdfEval.Radiance;
        }
    }

    if (ForwardFlagGet(gSettings.Flags, ForwardFlags::ePunctualLights))
    {
        E_direct += EvaluatePunctuals(rngInfo, hitInfo, wo, float4(hitInfo.HitPos, 1)); // TODO: If works, refactor float4 to float3
    }

    float3 E_ibl = 0.0f;
    if (ForwardFlagGet(gSettings.Flags, ForwardFlags::eEnvironmentMap))
    {
        float3 wi = reflect(-wo, hitInfo.Ns_ff);

        float3 F0 = lerp(0.04f, hitInfo.Mat.Albedo.rgb, hitInfo.Mat.Metallic);
        float3 F = F_Schlick(NdV, F0);

        float lod = hitInfo.Mat.Roughness * (gSettings.MaxCubemapMipMaps - 1);
        float3 envSample = gSkyBox.SampleLevel(gSamplerLinearWrap, wi, lod).rgb;

        float2 uv_brdfInt = saturate(float2(max(NdV, 0), hitInfo.Mat.Roughness));
        float2 brdfIntSample = gBrdfInt.SampleLevel(gSamplerLinearWrap, uv_brdfInt, 0).rg;

        E_ibl = envSample * (F * brdfIntSample.r + brdfIntSample.g);
        E_ibl = max(0, E_ibl);
    }

    float3 E_ambient = 0.0f;
    if (ForwardFlagGet(gSettings.Flags, ForwardFlags::eAmbient))
        E_ambient = hitInfo.Mat.Albedo.rgb * 0.1f;

    float3 E = E_direct + E_ibl + E_ambient;
    //float3 E = E_direct + E_ambient;
    //float3 E = E_direct;
    //float3 E = E_ibl;

    // Post-Processing
    {
        TMO_Reinhard tmo;
        ToneMapJodie(E, tmo);

        E = LRGB_to_SRGB_Exact(E);
    }

    gTexPrimal[pixelCoord] = float4(E, hitInfo.Mat.Albedo.a);
}