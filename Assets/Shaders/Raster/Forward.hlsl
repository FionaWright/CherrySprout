#include "Utils/CBVs.h"
#include "MicrofacetModels/MicrofacetUtils.hlsli"
#include "Utils/SharedUtils.h"
#include "Utils/Math/ShadingFrame.h"
#include "Scene/Material.h"
#include "Utils/Debug/Palette.h"
#include "PathTracing/Structs.h"
#include "Utils/Random.h"

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
};

ConstantBuffer<CbvForward_PerInstance> gCbvObject : register(b0, space0); // Push Constants
ConstantBuffer<CbvMatrices_VP> gMatricesVP : register(b1, space0);
ConstantBuffer<CbvForward> gSettings : register(b2, space0);

StructuredBuffer<Material> gMaterials : register(t0, space0);
Texture2D<float2> gBrdfInt : register(t1, space0);
TextureCube gSkyBox : register(t2, space0);
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

    float3 Li = 1.0f;

    float3 E_direct = 0.0f;
    if (NdL > 0.0f)
    {
        BxdfEval bxdfEval = BxDF::Evaluate(hitInfo, wo, wi, LobeTag::eAll);

        if (bxdfEval.PDF > 0.0f)
            E_direct += NdL * Li * bxdfEval.Radiance;
    }

    float3 F0 = lerp(0.04f, hitInfo.Mat.Albedo.rgb, hitInfo.Mat.Metallic);
    float3 F = F_Schlick(VdH, F0);

    float lod = hitInfo.Mat.Roughness * (gSettings.MaxCubemapMipMaps - 1);
    float3 envSample = gSkyBox.SampleLevel(gSamplerLinearWrap, wi_R, lod).rgb;

    float2 uv_brdfInt = saturate(float2(max(NdV, 0), hitInfo.Mat.Roughness));
    float2 brdfIntSample = gBrdfInt.SampleLevel(gSamplerLinearWrap, uv_brdfInt, 0).rg;
    float3 E_indirect = envSample * (F * brdfIntSample.r + brdfIntSample.g);
    E_indirect = max(0, E_indirect);

    // TODO: Shadow map

    float3 E_ambient = hitInfo.Mat.Albedo.rgb * 0.2f;

    float3 E = E_direct + E_indirect + E_ambient;
    //float3 E = E_direct;
    //float3 E = E_indirect;

    return float4(E, hitInfo.Mat.Albedo.a);
}