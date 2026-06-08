#ifndef H_PBR_H
#define H_PBR_H

bool IsReflect(inout RngInfo rngInfo, float iorCurrent, float iorNext, float NdV, bool isConductor, out float reflectProb)
{
    reflectProb = Fresnel_Maxwell(iorCurrent, iorNext, abs(NdV), isConductor);

    if (cDebugForceReflect)
        reflectProb = 1.0f;
    else if (!isConductor && CheckTIR(iorCurrent.Re, iorNext.Re, abs(NdV)))
        reflectProb = 1.0f;
    else if (cDebugForceRefract)
        reflectProb = 0.0f;

    float rReflectProb = Rand01(rngInfo);
    return reflectProb > rReflectProb;
}

bool IsSpecular(inout RngInfo rngInfo, float NdV, float3 F0, out float specProb)
{
    float3 F_select = F_Schlick(NdV, F0);

    specProb = Luminance(F_select); // kS
    specProb = clamp(specProb, 0.05f, 0.95f);

    if (cDebugForceSpecular)
        specProb = 1.0f;
    else if (cDebugForceDiffuse)
        specProb = 0.0f;

    float rSpecProb = Rand01(rngInfo);
    return rSpecProb < specProb;
}

void BxDF::Sample(
    inout RngInfo rngInfo,
    HitInfo hitInfo,
    float3 wo,

    out float3 wi,
    out float3 f,
    out float pdf
)
{
    ShadingFrame sframe = CreateShadingFrame(hitInfo.Ns_ff);

    float3 N_s = float3(0, 0, 1);
    float3 V_s = sframe.ToLocal(wo);

    float NdV = SSpaceCosTheta(V_s);

    float3 F0 = lerp(float3(0.04, 0.04, 0.04), hitInfo.Mat.Albedo.rgb, hitInfo.Mat.Metallic);

    float3 L_s = 0.0f;

    if (isGlass)
    {
        Complex iorMat = CreateComplex(ior, 0.0f);
        Complex iorCurrent = Ternary(entering, IOR_AIR, iorMat);
        Complex iorNext = Ternary(entering, iorMat, IOR_AIR);

        bool isConductor = false; // Metal glass not supported

        float reflectProb;
        bool isReflect = IsReflect(rngInfo, iorCurrent, iorNext, NdV, isConductor, reflectProb);

        if (isReflect)
        {
            BRDF_Specular(rngInfo, throughput, L_s, roughness, metalness, albedo, V_s, N_s, anisoDirAndStrength, sframe, F0, debug, hasDebugOutput);
            throughput /= max(0.001f, reflectProb);
        }
        else
        {
            BTDF(rngInfo, throughput, L_s, roughness, albedo, V_s, L_s, entering, iorCurrent.Re, iorNext.Re, sigmaA, hitDist, debug, hasDebugOutput);
            throughput /= max(0.001f, 1.0f - reflectProb);
        }

        wi = sframe.ToWorld(L_s);

#ifdef DEBUG_PT_INFO_OUTPUT
#     include "Debug/DebugInfoOutputBSDFT.hlsli"
#endif
        return;
    }

    L_sample = throughput * Li;

    float specProb;
    bool isSpecular = IsSpecular(rngInfo, NdV, F0, specProb);

    if (isSpecular)
    {
        BRDF_Specular(rngInfo, throughput, L_s, roughness, metalness, albedo, V_s, N_s, anisoDirAndStrength, sframe, F0, debug, hasDebugOutput);
        throughput /= max(0.001f, specProb);
    }
    else
    {
        BRDF_Diffuse(rngInfo, throughput, L_s, metalness, albedo, debug, hasDebugOutput);
        throughput /= max(0.001f, 1.0 - specProb);
    }

    wi = sframe.ToWorld(L_s);

    float u1 = Rand01(rngInfo);
    float u2 = Rand01(rngInfo);

    ShadingFrame sframe = CreateShadingFrame(hitInfo.Ns_ff);
    wi = RandHemisphereCosineWorld(u1, u2, sframe);

    float NdL = dot(hitInfo.Ns_ff, wi);
    f = hitInfo.Mat.Albedo.rgb;
    pdf = NdL / PI;
}

void BxDF::Evaluate(
    inout RngInfo rngInfo,
    HitInfo hitInfo,
    float3 wi,

    out float3 f,
    out float pdf
)
{
    float NdL = dot(hitInfo.Ns_ff, wi);
    f = hitInfo.Mat.Albedo.rgb;
    pdf = NdL / PI;
}

#endif