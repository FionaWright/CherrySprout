#include "Utils/CBVs.h"
#include "Utils/Math/Punctual.hlsli"
#include "MicrofacetModels/MicrofacetUtils.hlsli"
#include "Utils/SharedUtils.h"
#include "Utils/Math/ShadingFrame.h"
#include "Scene/Material.h"
#include "Utils/Debug/Palette.h"
#include "PathTracing/Structs.h"
#include "Utils/Random.h"
#include "PostProcessing/ToneMapping.hlsli"

#define BXDF_MODE BXDF_PBR
//#define BXDF_MODE BXDF_LAMBERTIAN
#include "BxDFs/GetBxDF.hlsli"

struct VsIn
{
    float3 position : POSITION;
    float3 normal : NORMAL;
    float2 uv : TEXCOORD0;
};

struct VsOut
{
    float4 position : SV_POSITION;
    float2 uv : TEXCOORD0;
    float3 normal : TEXCOORD1;
    float4 worldPos : TEXCOORD2;

    float3 shadowPos[SHADOW_MAP_MAX_CASCADES] : TEXCOORD3;
};

ConstantBuffer<CbvForward_PerInstance> gCbvObject     : register(b0, space0); // Push Constants
ConstantBuffer<CbvMatrices_VP> gMatricesVP            : register(b1, space0);
ConstantBuffer<CbvForward> gSettings                  : register(b2, space0);
ConstantBuffer<CbvShadowMapSample> gSettingsShadows   : register(b3, space0);

StructuredBuffer<Material> gMaterials                 : register(t0, space0);
StructuredBuffer<PunctualLight> gPunctuals            : register(t1, space0);
Texture2D<float2> gBrdfInt                            : register(t2, space0);
TextureCube gSkyBox                                   : register(t3, space0);
Texture2D<float> gShadowMapDir                        : register(t4, space0);
//TextureCube gIrradiance                               : register(t1, space0);

Texture2D<float4> gSceneTextures[] : register(t0, space1);

SamplerState gSamplerLinearWrap : register(s0, space0);
SamplerState gSamplerPointClamp : register(s1, space0);

#include "PathTracing/HitInfo/ApplyMaterialTextures.hlsli"
#include "Raster/Shadows.hlsli"

VsOut VSMain(VsIn input)
{
    VsOut output;

    output.normal = normalize(mul((float3x3)gCbvObject.MTI, (float3)input.normal));
    output.uv = input.uv;

    float4 pos = float4(input.position, 1.0f);
    output.worldPos = mul(gCbvObject.M, pos);
    pos = mul(gMatricesVP.V, output.worldPos);
    output.position = mul(gMatricesVP.P, pos);

    if (ForwardFlagGet(gSettings.Flags, ForwardFlags::eShadowMaps))
    {
        float3 normalBias = output.normal * gSettingsShadows.NormalBias;
        float4 biasedPos = float4(output.worldPos.xyz + normalBias, output.worldPos.w);
        for (uint i = 0; i < gSettingsShadows.CascadeCount; i++)
        {
            float4 shadowPos = mul(gSettingsShadows.ShadowMatrices[i], biasedPos);
            output.shadowPos[i] = shadowPos.xyz / shadowPos.w;
            output.shadowPos[i].xy = RemapStoU(output.shadowPos[i].xy);
        }
    }

    return output;
}

float4 PSMain(VsOut input) : SV_TARGET
{
    float3 Ns = normalize(input.normal);
    float2 uv = input.uv;
    uv.y = 1 - uv.y;

    HitInfo hitInfo;
    {
        hitInfo.Mat = gMaterials[gCbvObject.MaterialIdx];

        ApplyNormalMap(uv, hitInfo.Mat.TexIdxNormal, Ns, true);

        hitInfo.Ng_ff = Ns;
        hitInfo.Ns_ff = Ns;
        hitInfo.SFrame = CreateShadingFrame(Ns);
        hitInfo.UV = uv;
        hitInfo.IsEntering = true;
        hitInfo.RayT = length(input.worldPos.xyz - gSettings.CameraPosition);
        hitInfo.HitPos = input.worldPos.xyz;

        ApplyMaterialTextures(hitInfo);
    }

    float3 wo = normalize(gSettings.CameraPosition - input.worldPos.xyz);

    float NdV = saturate(dot(Ns, wo));

    //float3 irradianceIblSample = gIrradiance.SampleLevel(gSamplerLinearWrap, Ns, 0).rgb;
    //float3 diffuseIBL = f_bxdf * irradianceIblSample;

    RngInfo rngInfo = InitializeRngInfo(input.position.xy, 0, gSettings.FrameIdx, 12005);

    float3 E_direct = 0.0f;
    if (ForwardFlagGet(gSettings.Flags, ForwardFlags::eDirectionalLight))
    {
        float3 dirLightRadiance = 1.0f;

        float3 wi = -gSettings.DirLightDir;
        float NdL = max(1e-6f, dot(Ns, wi));
        if (NdL > 0.0f)
        {
            BxdfEval bxdfEval = BxDF::Evaluate(hitInfo, wo, wi, LobeTag::eAll);

            float shadowFactor = 1.0f;
            if (ForwardFlagGet(gSettings.Flags, ForwardFlags::eShadowMaps))
                shadowFactor = GetShadowFactorDirectional(rngInfo, input.shadowPos);

            if (bxdfEval.PDF > 0.0f)
                E_direct += shadowFactor * NdL * dirLightRadiance * bxdfEval.Radiance;
        }
    }
    if (ForwardFlagGet(gSettings.Flags, ForwardFlags::ePunctualLights))
    {
        uint lightCount, _;
        gPunctuals.GetDimensions(lightCount, _);

        for (uint i = 0; i < lightCount; i++)
        {
            PunctualLight light = gPunctuals[i];

            LightEval lightEval;
            LightSample lightSample = EvaluatePunctualLight(light, hitInfo.HitPos, lightEval);

            float3 wi = lightEval.Direction;
            float NdL = max(1e-6f, dot(Ns, wi));
            if (NdL > 0.0f && any(lightEval.Radiance > 0))
            {
                if (light.Type == PunctualLightType::eDistant)
                {

                }
                else
                {

                }

                BxdfEval bxdfEval = BxDF::Evaluate(hitInfo, wo, wi, LobeTag::eAll);

                float shadowFactor = 1.0f; // TODO

                //float radMult = 100.0f;

                if (bxdfEval.PDF > 0.0f)
                    E_direct += shadowFactor * NdL * lightEval.Radiance * bxdfEval.Radiance;
            }
        }
    }

    float3 E_ibl = 0.0f;
    if (ForwardFlagGet(gSettings.Flags, ForwardFlags::eEnvironmentMap))
    {
        float3 wi = reflect(-wo, Ns);

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

    return float4(E, hitInfo.Mat.Albedo.a);
}