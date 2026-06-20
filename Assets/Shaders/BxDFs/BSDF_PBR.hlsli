#ifndef H_PBR_H
#define H_PBR_H

#include "Utils/Math/Fresnel.hlsli"
#include "Utils/Debug/Assert.hlsli"
#include "MicrofacetModels/GetMM.hlsli"

#include "BxDFs/Lobes/LambertianLobe.hlsli"
#include "BxDFs/Lobes/SpecularLobe.hlsli"
#include "BxDFs/Lobes/TransmissiveLobe.hlsli"

bool IsReflect(inout RngInfo rngInfo, float iorCurrent, float iorNext, float NdV, out float reflectProb)
{
    reflectProb = Fresnel_Dielectric_Unpolarized(iorCurrent, iorNext, abs(NdV));

    if (DEBUG_ENABLED(ForceReflect))
        reflectProb = 1.0f;
    else if (CheckTIR(iorCurrent, iorNext, abs(NdV)))
        reflectProb = 1.0f;
    else if (DEBUG_ENABLED(ForceRefract))
        reflectProb = 0.0f;

    return Rand01(rngInfo) < reflectProb;
}

bool IsSpecular(inout RngInfo rngInfo, float NdV, float3 F0, out float specProb)
{
    float3 F_select = F_Schlick(NdV, F0);

    specProb = Luminance(F_select); // kS
    specProb = clamp(specProb, 0.05f, 0.95f);

    if (DEBUG_ENABLED(ForceSpecular))
        specProb = 1.0f;
    else if (DEBUG_ENABLED(ForceDiffuse))
        specProb = 0.0f;

    return Rand01(rngInfo) < specProb;
}

void BxDF::Sample(
    inout RngInfo rngInfo,
    inout PathState pathState,
    HitInfo hitInfo,
    float3 wo,

    out float3 wi,
    out float3 f,
    out float pdf
)
{
    float3 N_s = float3(0, 0, 1);
    float3 V_s = hitInfo.SFrame.ToLocal(wo);

    float NdV = SSpaceCosTheta(V_s);

    float3 F0 = lerp(float3(0.04, 0.04, 0.04), hitInfo.Mat.Albedo.rgb, hitInfo.Mat.Metallic);

    float u1 = Rand01(rngInfo);
    float u2 = Rand01(rngInfo);

    MicrofacetModel mm;
    InitializeMM(mm, hitInfo.Mat.Roughness, rngInfo, V_s);
    if (FEATURE_ENABLED(Anisotropy))
        InitializeMMAniso(mm, hitInfo);

    float3 H_s = mm.Sample(u1, u2);
    float VdH = dot(H_s, V_s);

    DBG_OUTPUT3(H_s,                             H_s);
    DBG_OUTPUT3(hitInfo.SFrame.ToWorld(H_s),     H_w);
    DBG_OUTPUT1(mm.m_alpha,                      Alpha);
    DBG_OUTPUT1(mm.D(H_s),                       D);

    float3 L_s = 0.0f;

    if (FEATURE_ENABLED(GlassMaterials) && Rand01(rngInfo) < hitInfo.Mat.TransmissionFactor)
    {
        float iorCurrent =  hitInfo.IsEntering ? IOR_N_AIR          : hitInfo.Mat.IOR_N;
        float iorNext =     hitInfo.IsEntering ? hitInfo.Mat.IOR_N  : IOR_N_AIR;

        DBG_ASSERT_ZERO(hitInfo.Mat.Metallic, NO_METAL_GLASS);
        DBG_ASSERT_ZERO(hitInfo.Li          , NO_EMISSIVE_GLASS);

        float reflectProb;
        bool isReflect = IsReflect(rngInfo, iorCurrent, iorNext, VdH, reflectProb);

        if (isReflect)
        {
            SpecularLobe_Sample(rngInfo, hitInfo, mm, V_s, N_s, H_s, F0, L_s, f, pdf);

            pdf *= reflectProb;

            DBG_ASSERT_VALUE(f, BxDF_PBR_GLASS_SPEC_F);
        }
        else
        {
            TransmissiveLobe_Sample(rngInfo, hitInfo, mm, V_s, N_s, H_s, iorCurrent, iorNext, L_s, f, pdf);

            pdf *= 1.0f - reflectProb;
        }

        wi = hitInfo.SFrame.ToWorld(L_s);
        return;
    }

    float specProb;
    bool isSpecular = IsSpecular(rngInfo, VdH, F0, specProb);

    if (isSpecular)
    {
        SpecularLobe_Sample(rngInfo, hitInfo, mm, V_s, N_s, H_s, F0, L_s, f, pdf);

        if (pdf == 1.0f)
            pathState.LastRayDiracDelta = true;

        pdf *= specProb;
    }
    else
    {
        LambertianLobe_Sample(rngInfo, hitInfo, L_s, f, pdf);

        f *= (1.0 - hitInfo.Mat.Metallic);
        pdf *= 1.0f - specProb;
    }

    wi = hitInfo.SFrame.ToWorld(L_s);
}

void BxDF::Evaluate(
    inout RngInfo rngInfo,
    HitInfo hitInfo,
    float3 wo,
    float3 wi,

    out float3 f,
    out float pdf
)
{
    float3 N_s = float3(0, 0, 1);
    float3 V_s = hitInfo.SFrame.ToLocal(wo);
    float3 L_s = hitInfo.SFrame.ToLocal(wi);
    float3 H_s = normalize(V_s + L_s);

    float NdV = SSpaceCosTheta(V_s);

    float3 F0 = lerp(float3(0.04, 0.04, 0.04), hitInfo.Mat.Albedo.rgb, hitInfo.Mat.Metallic);

    MicrofacetModel mm;
    InitializeMM(mm, hitInfo.Mat.Roughness, rngInfo, V_s);
    if (FEATURE_ENABLED(Anisotropy))
        InitializeMMAniso(mm, hitInfo);

    float VdH = dot(H_s, V_s);

    DBG_OUTPUT3(H_s,                             H_s);
    DBG_OUTPUT3(hitInfo.SFrame.ToWorld(H_s),     H_w);
    DBG_OUTPUT1(mm.m_alpha,                      Alpha);
    DBG_OUTPUT1(mm.D(H_s),                       D);

    if (FEATURE_ENABLED(GlassMaterials) && Rand01(rngInfo) < hitInfo.Mat.TransmissionFactor)
    {
        float iorCurrent =  hitInfo.IsEntering ? IOR_N_AIR          : hitInfo.Mat.IOR_N;
        float iorNext =     hitInfo.IsEntering ? hitInfo.Mat.IOR_N  : IOR_N_AIR;

        DBG_ASSERT_ZERO(hitInfo.Mat.Metallic, NO_METAL_GLASS);
        DBG_ASSERT_ZERO(hitInfo.Li          , NO_EMISSIVE_GLASS);

        float reflectProb;
        bool isReflect = IsReflect(rngInfo, iorCurrent, iorNext, VdH, reflectProb);

        if (isReflect)
        {
            SpecularLobe_Evaluate(rngInfo, hitInfo, mm, V_s, N_s, H_s, L_s, F0, f, pdf);

            pdf *= reflectProb;

            DBG_ASSERT_VALUE(f, BxDF_PBR_GLASS_SPEC_F);
        }
        else
        {
            TransmissiveLobe_Evaluate(rngInfo, hitInfo, mm, V_s, N_s, H_s, L_s, iorCurrent, iorNext, f, pdf);

            pdf *= 1.0f - reflectProb;
        }
        return;
    }

    float specProb;
    bool isSpecular = IsSpecular(rngInfo, VdH, F0, specProb);

    if (isSpecular)
    {
        SpecularLobe_Evaluate(rngInfo, hitInfo, mm, V_s, N_s, H_s, L_s, F0, f, pdf);

        pdf *= specProb;
    }
    else
    {
        LambertianLobe_Evaluate(rngInfo, hitInfo, wi, f, pdf);

        f *= (1.0 - hitInfo.Mat.Metallic);
        pdf *= 1.0f - specProb;
    }
}

#endif