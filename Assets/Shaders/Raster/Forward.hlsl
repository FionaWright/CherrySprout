#include "Utils/CBVs.h"
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

#define SHADOW_MAP_ENABLED 1 // TODO: GUI

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

#if SHADOW_MAP_ENABLED
    float3 shadowPos[SHADOW_MAP_MAX_CASCADES] : TEXCOORD3;
#endif
};

ConstantBuffer<CbvForward_PerInstance> gCbvObject : register(b0, space0); // Push Constants
ConstantBuffer<CbvMatrices_VP> gMatricesVP : register(b1, space0);
ConstantBuffer<CbvForward> gSettings : register(b2, space0);
ConstantBuffer<CbvShadowMapSample> gSettingsShadows : register(b3, space0);

StructuredBuffer<Material> gMaterials : register(t0, space0);
Texture2D<float2> gBrdfInt : register(t1, space0);
TextureCube gSkyBox : register(t2, space0);
Texture2D<float> gShadowMapDir : register(t3, space0);
//TextureCube gIrradiance : register(t1, space0);

Texture2D<float4> gSceneTextures[] : register(t0, space1);

SamplerState gSamplerLinearWrap : register(s0, space0);

#include "PathTracing/HitInfo/ApplyMaterialTextures.hlsli"

VsOut VSMain(VsIn input)
{
    VsOut output;

    output.normal = normalize(mul((float3x3)gCbvObject.MTI, (float3)input.normal));
    output.uv = input.uv;

    float4 pos = float4(input.position, 1.0f);
    output.worldPos = mul(gCbvObject.M, pos);
    pos = mul(gMatricesVP.V, output.worldPos);
    output.position = mul(gMatricesVP.P, pos);

#if SHADOW_MAP_ENABLED
    float3 normalBias = output.normal * gSettingsShadows.NormalBias;
    float4 biasedPos = float4(output.worldPos.xyz + normalBias, output.worldPos.w);
    for (uint i = 0; i < gSettingsShadows.CascadeCount; i++)
    {
        float4 shadowPos = mul(gSettingsShadows.ShadowMatrices[i], biasedPos);
        output.shadowPos[i] = shadowPos.xyz / shadowPos.w;
        output.shadowPos[i].xy = RemapStoU(output.shadowPos[i].xy);
    }
#endif

    return output;
}

float4 PSMain(VsOut input) : SV_TARGET
{
    float3 Ns = normalize(input.normal);
    float2 uv = input.uv;
    uv.y = 1 - uv.y;

    HitInfo hitInfo;
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

    float3 wo = normalize(gSettings.CameraPosition - input.worldPos.xyz);
    float3 wi = -gSettings.DirLightDir;
    float3 wi_R = reflect(-wo, Ns);

    float3 H = normalize(wo + wi);

    float VdH = dot(H, wo);
    float NdL = max(1e-6f, dot(Ns, wi));
    float NdV = saturate(dot(Ns, wo));

    //float3 irradianceIblSample = gIrradiance.SampleLevel(gSamplerLinearWrap, Ns, 0).rgb;
    //float3 diffuseIBL = f_bxdf * irradianceIblSample;

    // TODO: Put into new file
    float shadowFactor = 1.0f;
#if SHADOW_MAP_ENABLED
    {
        uint cascadeIdx = 0; // TODO
        float blueNoise = 1.0f; // TODO

        float3 shadowCoords = input.shadowPos[cascadeIdx];
        shadowCoords.y = 1 - shadowCoords.y;
        //shadowCoords.x = gSettingsShadows.ShadowWidthPercent * (shadowCoords.x + cascadeIdx);

        float pixelDepth = shadowCoords.z + gSettingsShadows.BiasDepth;

        //return float4((pixelDepth-0.94f), 0,0,1);

        shadowFactor = 0.0f;
        for (uint i = 0; i < gSettingsShadows.PcfSampleCount; i++)
        {
            float2 offset = float2(0,0);

            // PCF
            if (false)
            {
                uint poissonIndex = (i * blueNoise * POISSON_DISC_COUNT) % POISSON_DISC_COUNT;
                offset = i & 1 ? gSettingsShadows.PoissonDisc[poissonIndex].xy : gSettingsShadows.PoissonDisc[poissonIndex].zw;

                // Rotate
                if (false)
                {
                    float rng = 0.0f; // TODO
                    float angle = rng * 6.2831853;

                    float s, c;
                    sincos(angle, s, c);

                    offset = float2(
                        offset.x * c - offset.y * s,
                        offset.x * s + offset.y * c
                    );
                }

                offset *= gSettingsShadows.PcfRadius;
            }

            float2 uv_shadow = shadowCoords.xy + (offset * gSettingsShadows.TexelSize);
            float shadowMapDepth = gShadowMapDir.Sample(gSamplerLinearWrap, uv_shadow).r; // TODO: Is linear wrong?

            //return float4((shadowMapDepth)>0, 0,0,1);
            //return float4(uv_shadow,0,1);
            //return float4(shadowCoords.xy,0,1);

            shadowFactor += pixelDepth >= shadowMapDepth;
        }
        shadowFactor /= float(gSettingsShadows.PcfSampleCount);
    }
#endif

    //return float4(shadowFactor.xxx,1);

    float3 Li = 1.0f;

    float3 E_direct = 0.0f;
    if (NdL > 0.0f)
    {
        BxdfEval bxdfEval = BxDF::Evaluate(hitInfo, wo, wi, LobeTag::eAll);

        if (bxdfEval.PDF > 0.0f)
            E_direct = shadowFactor * NdL * Li * bxdfEval.Radiance;
    }

    float3 E_indirect;
    {
        float3 F0 = lerp(0.04f, hitInfo.Mat.Albedo.rgb, hitInfo.Mat.Metallic);
        float3 F = F_Schlick(VdH, F0);

        float lod = hitInfo.Mat.Roughness * (gSettings.MaxCubemapMipMaps - 1);
        float3 envSample = gSkyBox.SampleLevel(gSamplerLinearWrap, wi_R, lod).rgb;

        float2 uv_brdfInt = saturate(float2(max(NdV, 0), hitInfo.Mat.Roughness));
        float2 brdfIntSample = gBrdfInt.SampleLevel(gSamplerLinearWrap, uv_brdfInt, 0).rg;

        E_indirect = envSample * (F * brdfIntSample.r + brdfIntSample.g);
        E_indirect = max(0, E_indirect);
    }

    float3 E_ambient = hitInfo.Mat.Albedo.rgb * 0.2f;

    float3 E = E_direct + E_indirect + E_ambient;
    //float3 E = E_direct + E_ambient;
    //float3 E = E_direct;
    //float3 E = E_indirect;

    // Post-Processing
    {
        TMO_Reinhard tmo;
        ToneMapJodie(E, tmo);

        E = LRGB_to_SRGB_Exact(E);
    }

    return float4(E, hitInfo.Mat.Albedo.a);
}