#include "Utils/CBVs.h"
#include "MicrofacetModels/MicrofacetUtils.hlsli"
#include "Utils/SharedUtils.h"
#include "Utils/Math/ShadingFrame.h"
#include "Scene/Material.h"
#include "Utils/Debug/Palette.h"
#include "PathTracing/Structs.h"
#include "Utils/Random.h"

#define BXDF_MODE BXDF_PBR
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

RaytracingAccelerationStructure gTLAS : register(t0, space0);
StructuredBuffer<Material> gMaterials : register(t1, space0);

//Texture2D<float2> gBrdfInt : register(t0, space0);
//TextureCube gEnvMap : register(t0, space0);
//TextureCube gIrradiance : register(t2, space0);

Texture2D<float4> gSceneTextures[] : register(t0, space1);

SamplerState gSampler : register(s0, space0);

#include "Raster/RayTracedShadows.hlsli"

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
    float3 Ng = normalize(input.normal);
    float2 uv = input.uv;
    uv.y *= gCbvObject.ScaleY;

    Material mat = gMaterials[gCbvObject.MaterialIdx];
    mat.Roughness = 0.0f;
    mat.Metallic = 0.1f;

    float3 worldPos = input.worldPos.xyz / input.worldPos.w;

    float3 albedoSample = gSceneTextures[mat.TexIdxAlbedo].Sample(gSampler, uv).rgb;

    if (Ng.y >= 0.9f)
        albedoSample.rgb = float3(0.3, 0.3, 0.3);

    mat.Albedo.rgb = albedoSample;

    HitInfo hitInfo;
    hitInfo.Mat = mat;
    hitInfo.Ng_ff = Ng;
    hitInfo.Ns_ff = Ng;
    hitInfo.SFrame = CreateShadingFrame(Ng);
    hitInfo.Emission = 0.0f;
    hitInfo.UV = uv;
    hitInfo.IsEntering = true;
    hitInfo.RayT = 0.0f; // TODO
    hitInfo.HitPos = worldPos;

    float3 wo = normalize(gSettings.CameraPosition - worldPos);
    float3 wi = normalize(-gSettings.DirLightDir);

    float3 Li = 5.0f;

    float3 E_direct_sum = 0.0f;

    BxDF bxdf;
    float3 f_bxdf;
    float pdf_bxdf;
    bxdf.Evaluate(hitInfo, wo, wi, f_bxdf, pdf_bxdf);

    float shadowFactor = 0.0f;
    {
        float3 up = abs(wi.y) < 0.999f
            ? float3(0, 1, 0)
            : float3(1, 0, 0);

        float3 wiT = normalize(cross(up, wi));
        float3 wiB = cross(wi, wiT);

//#define NUM_SHADOW_SAMPLES 32
#define NUM_SHADOW_SAMPLES 32

        RngInfo rngInfo = InitializeRngInfo(input.position.xy, 0, gSettings.FrameIdx, 1205);

        [unroll]
        for (int i = 0; i < NUM_SHADOW_SAMPLES; i++)
        {
            float u1 = Rand01(rngInfo);
            float u2 = Rand01(rngInfo);

            //float angularRadius = 0.01f;
            float angularRadius = 0.01f;

            float cosThetaMax = cos(angularRadius);

            float cosTheta = lerp(1.0f, cosThetaMax, u1);
            float sinTheta = sqrt(1.0f - cosTheta * cosTheta);

            float phi = 2.0f * PI * u2;

            float3 wiMod =
                wiT   * (cos(phi) * sinTheta) +
                wiB * (sin(phi) * sinTheta) +
                wi  * cosTheta;

            wiMod = normalize(wiMod);

            float NdL = dot(Ng, wiMod);

            if (NdL <= 0.0f)
                continue;

            float3 rayOrigin = worldPos + Ng * EPSILON;
            TraceRayShadow(rayOrigin, wiMod, INF, shadowFactor);

            E_direct_sum += max(0.0f, NdL) * shadowFactor * Li * f_bxdf;
        }
    }

    float3 E_direct = E_direct_sum / float(NUM_SHADOW_SAMPLES);

    float3 E_ambient = mat.Albedo.rgb * 0.2f;

    float3 E = E_direct + E_ambient;

    return float4(E, mat.Albedo.a);
}