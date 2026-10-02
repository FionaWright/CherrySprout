#include "Utils/CBVs.h"
#include "MicrofacetModels/MicrofacetUtils.hlsli"
#include "Utils/SharedUtils.h"
#include "Utils/Math/ShadingFrame.h"
#include "Scene/Material.h"

struct VsIn
{
    float3 position : POSITION;
    float3 normal : NORMAL;
    float2 uv : TEXCOORD0;
};

struct VsOut
{
    float4 position : SV_POSITION;
    float3 normal : TEXCOORD0;
    float2 uv : TEXCOORD1;
};

ConstantBuffer<CbvForward_PerInstance> gCbvObject : register(b0); // Push Constants
ConstantBuffer<CbvMatrices_VP> gMatricesVP : register(b1);
ConstantBuffer<CbvForward> gSettings : register(b2);

//Texture2D<float2> gBrdfInt : register(t0);
TextureCube gEnvMap : register(t0);
//TextureCube gIrradiance : register(t2);

Texture2D<float4> gSceneTextures[] : register(t0, space1);

SamplerState gSampler : register(s0);

VsOut VSMain(VsIn input)
{
    VsOut output;

    output.normal = normalize(mul((float3x3)gCbvObject.MTI, (float3)input.normal));
    output.uv = input.uv;

    float4 pos = float4(input.position, 1.0f);
    float4 worldPos = mul(gCbvObject.M, pos);
    pos = mul(gMatricesVP.V, worldPos);
    output.position = mul(gMatricesVP.P, pos);

    return output;
}

float4 PSMain(VsOut input) : SV_TARGET
{
    //float3 bumpSample = gTextures[gMaterial.TexIdxNormal].SampleLevel(gSampler, input.uv, 0).rgb * 2.0f - 1.0f;
    //bumpSample.y = -bumpSample.y; // DX convention

    float3 Ns = normalize(input.normal);
    //ShadingFrame bumpFrame = CreateShadingFrame(Ns);
    //float3 N_w = bumpFrame.ToWorld(bumpSample);

    return float4(Ns, 1);
/*

    float4 albedo = gTextures[gMaterial.TexIdxAlbedo].Sample(gSampler, input.uv).rgba;
	float4 albedoGamma = float4(pow(albedo.rgb, 2.2f), albedo.a);

    float2 roughMet = gTextures[gMaterial.TexIdxRoughMet].Sample(gSampler, input.uv).gb;
    float3 emission = gTextures[gMaterial.TexIdxEmissive].Sample(gSampler, input.uv).rgb;

    float3 irradianceIblSample = gIrradiance.SampleLevel(gSampler, N_w, 0).rgb;

    float3 V = normalize(input.viewDir);
    float3 L = normalize(-gSettings.DirLightDir); // Surface to Light Vector
    float3 H = normalize(L + V);

    float NdL = saturate(dot(N_w, L));
    float NdV = saturate(dot(N_w, V));
    float NdH = saturate(dot(N_w, H));
    float HdV = saturate(dot(H, V));

    float roughness = roughMet.r; // TODO: MaterialData CBV
    float metalness = roughMet.g;
    float3 F0 = lerp(0.04f, albedoGamma.rgb, metalness);

    float D = D_GGX(NdH, roughness);
    float3 F = F_Schlick(HdV, F0);
    float G = G_SmithFast(NdL, NdV, roughness);

    float3 kS = max(0.0f, F);
    float3 kD = 1.0f - kS;

    float3 specularBrdf = (D * F * G) / max(0.001f, 4.0f * NdV);
    // specularBrdf *= dirLightColor;
    specularBrdf *= irradianceIblSample;
    specularBrdf = max(0.0f, specularBrdf);

    float3 diffuseBrdf = kD * albedoGamma.rgb * NdL;
	// diffuseBrdf /= PI; // ?
    // diffuseBrdf *= dirLightColor;
    diffuseBrdf *= irradianceIblSample;
    diffuseBrdf *= 1.0f - metalness;

    float3 combinedBrdf = diffuseBrdf + specularBrdf;
    float3 Lo = pow(combinedBrdf, 1.0f / 2.2f);

    float3 R = reflect(-V, N_w);
    float lod = roughness * (gSettings.MaxCubemapMipMaps - 1);
    float3 envSample = gEnvMap.SampleLevel(gSampler, R, lod).rgb;
    //envSample = pow(envSample, 2.2f);

    float2 brdfIntSample = gBrdfInt.SampleLevel(gSampler, saturate(float2(max(NdV, 0), roughness)), 0).rg;
    float3 indirectSpecular = envSample * (F * brdfIntSample.r + brdfIntSample.g) * kS;
    indirectSpecular = max(0, indirectSpecular);
    float3 combinedBrdfWithIndirect = combinedBrdf + indirectSpecular;
    float3 LoWithIndirect = pow(combinedBrdfWithIndirect, 1.0f / 2.2f);

    switch (gSettings.Mode)
    {
    case ePosition:
        return float4(input.position.xyz / input.position.w, 1);
	case eNormalsVertex:
		return float4(input.normal, 1);
    case eNormalsBumped:
        return float4(N_w, 1);
    case eTangent:
        return float4(bumpFrame.T, 1);
    case eBinormal:
        return float4(bumpFrame.B, 1);
    case eUV:
        return float4(input.uv, 0, 1);
    case eDirLighting:
        return float4(NdL.xxx, 1);
    case eTex:
        return albedo;
    case eDirLightingTex:
        return albedo * float4(NdL.xxx, 1);
    case eRoughness:
        return float4(roughness.xxx, 1);
    case eMetalness:
        return float4(metalness.xxx, 1);
    case eEmission:
        return float4(emission, 1);
    case eViewDir:
        return float4(V, 1);
    case eHalfVec:
        return float4(H, 1);
    case eNDF:
        return float4(D.xxx, 1);
	case eFresnel:
		return float4(F, 1);
    case eGeometricMasking:
        return float4(G.xxx, 1);
    case eReflection:
        return float4(envSample, 1);
    case eIrradianceIBL:
        return float4(irradianceIblSample, 1);
    case eMicrofacetSpecular:
        return float4(pow(specularBrdf, 1.0f / 2.2f), 1);
    case eMicrofacetDiffuse:
        return float4(pow(diffuseBrdf, 1.0f / 2.2f), 1);
    case eMicrofacetIndirect:
        return float4(pow(indirectSpecular, 1.0f / 2.2f), 1);
    case eMicrofacetLo:
        return float4(Lo, 1);
    case eMicrofacetLoWithIndirect:
        return float4(LoWithIndirect, 1);
    }
    return float4(0, 1, 1, 1);
*/
}