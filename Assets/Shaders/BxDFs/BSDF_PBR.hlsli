#ifndef H_PBR_H
#define H_PBR_H

#include "Utils/Math/Fresnel.hlsli"
#include "Utils/Debug/Assert.hlsli"
#include "MicrofacetModels/GetMM.hlsli"

#include "BxDFs/Lobes/LambertianLobe.hlsli"
#include "BxDFs/Lobes/SpecularLobe.hlsli"
#include "BxDFs/Lobes/TransmissiveLobe.hlsli"

float GetReflectProb(float iorCurrent, float iorNext, float NdV)
{
    if (DEBUG_ENABLED(ForceReflect))
        return 1.0f;
    else if (CheckTIR(iorCurrent, iorNext, abs(NdV)))
        return 1.0f;
    else if (DEBUG_ENABLED(ForceRefract))
        return 0.0f;

    return Fresnel_Dielectric_Unpolarized(iorCurrent, iorNext, abs(NdV));
}

float GetSpecularProb(float NdV, float3 F0)
{
    if (DEBUG_ENABLED(ForceSpecular))
        return 1.0f;
    else if (DEBUG_ENABLED(ForceDiffuse))
        return 0.0f;

    return 0.5f;

    float3 F_select = F_Schlick(NdV, F0);

    float specProb = Luminance(F_select); // kS
    return clamp(specProb, 0.05f, 0.95f);
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

    DBG_ASSERT_VALUE(H_s,                        BxDF_PBR_H_S);

    DBG_OUTPUT3(H_s,                             H_s);
    DBG_OUTPUT3(hitInfo.SFrame.ToWorld(H_s),     H_w);
    DBG_OUTPUT1(mm.m_alpha,                      Alpha);
    DBG_OUTPUT1(mm.D(H_s),                       D);

    float3 L_s = 0.0f;

    if (FEATURE_ENABLED(GlassMaterials))
    {
        if (Rand01(rngInfo) < hitInfo.Mat.TransmissionFactor)
        {
            float iorCurrent =  hitInfo.IsEntering ? IOR_N_AIR          : hitInfo.Mat.IOR_N;
            float iorNext =     hitInfo.IsEntering ? hitInfo.Mat.IOR_N  : IOR_N_AIR;

            DBG_ASSERT_ZERO(hitInfo.Mat.Metallic, NO_METAL_GLASS);
            DBG_ASSERT_ZERO(hitInfo.Li          , NO_EMISSIVE_GLASS);

            float reflectProb = GetReflectProb(iorCurrent, iorNext, VdH);
            bool isReflect = Rand01(rngInfo) < reflectProb;

            DBG_OUTPUT1(reflectProb,                      ReflectProb);

            if (isReflect)
            {
                bool isDiracDelta;
                SpecularLobe_Sample(hitInfo, mm, V_s, N_s, H_s, F0, L_s, f, pdf, isDiracDelta);

                pathState.LastRayDiracDelta = isDiracDelta;

                pdf *= reflectProb;

                DBG_ASSERT_VALUE(f,         BxDF_PBR_GLASS_SPEC_F);
                DBG_ASSERT_VALUE(pdf,       BxDF_PBR_GLASS_SPEC_PDF);
                DBG_ASSERT_VALUE(L_s,       BxDF_PBR_GLASS_SPEC_L_S);
            }
            else
            {
                TransmissiveLobe_Sample(hitInfo, mm, V_s, N_s, H_s, iorCurrent, iorNext, L_s, f, pdf);

                pdf *= 1.0f - reflectProb;

                DBG_ASSERT_VALUE(f,         BxDF_PBR_GLASS_REFRACT_F);
                DBG_ASSERT_VALUE(pdf,       BxDF_PBR_GLASS_REFRACT_PDF);
                DBG_ASSERT_VALUE(L_s,       BxDF_PBR_GLASS_REFRACT_L_S);
            }

            pdf *= hitInfo.Mat.TransmissionFactor;

            wi = hitInfo.SFrame.ToWorld(L_s);
            return;
        }
    }

    float specProb = GetSpecularProb(NdV, F0);
    bool isSpecular = Rand01(rngInfo) < specProb;

    DBG_OUTPUT1(specProb,                      SpecProb);

    if (isSpecular)
    {
        bool isDiracDelta;
        SpecularLobe_Sample(hitInfo, mm, V_s, N_s, H_s, F0, L_s, f, pdf, isDiracDelta);

        pathState.LastRayDiracDelta = isDiracDelta;

        pdf *= specProb;

        DBG_ASSERT_VALUE(f,           BxDF_PBR_SPEC_F);
        DBG_ASSERT_VALUE(pdf,         BxDF_PBR_SPEC_PDF);
        DBG_ASSERT_VALUE(L_s,         BxDF_PBR_SPEC_L_S);
    }
    else
    {
        LambertianLobe_Sample(hitInfo, u1, u2, L_s, f, pdf);

        f *= (1.0 - hitInfo.Mat.Metallic);
        pdf *= 1.0f - specProb;

        DBG_ASSERT_VALUE(f,           BxDF_PBR_DIFF_F);
        DBG_ASSERT_VALUE(pdf,         BxDF_PBR_Diff_PDF);
        DBG_ASSERT_VALUE(L_s,         BxDF_PBR_Diff_L_S);
    }

    if (FEATURE_ENABLED(GlassMaterials))
        pdf *= 1.0f - hitInfo.Mat.TransmissionFactor;

    wi = hitInfo.SFrame.ToWorld(L_s);
}

void BxDF::Evaluate(
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
    InitializeMM(mm, hitInfo.Mat.Roughness, V_s);
    if (FEATURE_ENABLED(Anisotropy))
        InitializeMMAniso(mm, hitInfo);

    float VdH = dot(H_s, V_s);

    DBG_OUTPUT3(H_s,                             H_s);
    DBG_OUTPUT3(hitInfo.SFrame.ToWorld(H_s),     H_w);
    DBG_OUTPUT1(mm.m_alpha,                      Alpha);
    DBG_OUTPUT1(mm.D(H_s),                       D);

    f = 0;
    pdf = 0;

    if (FEATURE_ENABLED(GlassMaterials) && hitInfo.Mat.TransmissionFactor > 0)
    {
        float iorCurrent =  hitInfo.IsEntering ? IOR_N_AIR          : hitInfo.Mat.IOR_N;
        float iorNext =     hitInfo.IsEntering ? hitInfo.Mat.IOR_N  : IOR_N_AIR;

        DBG_ASSERT_ZERO(hitInfo.Mat.Metallic, NO_METAL_GLASS);
        DBG_ASSERT_ZERO(hitInfo.Li          , NO_EMISSIVE_GLASS);

        float reflectProb = GetReflectProb(iorCurrent, iorNext, VdH);

        DBG_OUTPUT1(reflectProb,                      ReflectProb);

        float3 f_trans = 0;
        float pdf_trans = 0;

        {
            float3 f_reflect;
            float pdf_reflect;
            SpecularLobe_Evaluate(hitInfo, mm, V_s, N_s, H_s, L_s, F0, f_reflect, pdf_reflect);

            pdf_reflect *= reflectProb;

            if (DEBUG_ENABLED(ForceRefract))
                f_reflect = 0.0f;

            f_trans += f_reflect;
            pdf_trans += pdf_reflect;

            DBG_ASSERT_VALUE(f_reflect,         BxDF_PBR_GLASS_SPEC_F);
            DBG_ASSERT_VALUE(pdf_reflect,       BxDF_PBR_GLASS_SPEC_PDF);
        }

        {
            float3 f_refract;
            float pdf_refract;
            TransmissiveLobe_Evaluate(hitInfo, mm, V_s, N_s, H_s, L_s, iorCurrent, iorNext, f_refract, pdf_refract);

            pdf_refract *= 1.0f - reflectProb;

            if (DEBUG_ENABLED(ForceReflect))
                f_refract = 0.0f;

            f_trans += f_refract;
            pdf_trans += pdf_refract;

            DBG_ASSERT_VALUE(f_refract,         BxDF_PBR_GLASS_REFRACT_F);
            DBG_ASSERT_VALUE(pdf_refract,       BxDF_PBR_GLASS_REFRACT_PDF);
        }

        f_trans *= hitInfo.Mat.TransmissionFactor;
        pdf_trans *= hitInfo.Mat.TransmissionFactor;

        f += f_trans;
        pdf += pdf_trans;
    }

    float specProb = GetSpecularProb(NdV, F0);

    DBG_OUTPUT1(specProb,                      SpecProb);

    float3 f_opaque = 0;
    float pdf_opaque = 0;
    {
        {
            float3 f_spec;
            float pdf_spec;
            SpecularLobe_Evaluate(hitInfo, mm, V_s, N_s, H_s, L_s, F0, f_spec, pdf_spec);

            if (DEBUG_ENABLED(ForceDiffuse) || DEBUG_ENABLED(ForceSpecular))
                f_spec *= specProb;
            pdf_spec *= specProb;

            f_opaque += f_spec;
            pdf_opaque += pdf_spec;

            DBG_ASSERT_VALUE(f_spec,           BxDF_PBR_SPEC_F);
            DBG_ASSERT_VALUE(pdf_spec,         BxDF_PBR_SPEC_PDF);
        }

        {
            float3 f_diff;
            float pdf_diff;
            LambertianLobe_Evaluate(hitInfo, L_s, f_diff, pdf_diff);

            f_diff *= (1.0 - hitInfo.Mat.Metallic);
            if (DEBUG_ENABLED(ForceDiffuse) || DEBUG_ENABLED(ForceSpecular))
                f_diff *= 1.0f - specProb;
            pdf_diff *= 1.0f - specProb;

            f_opaque += f_diff;
            pdf_opaque += pdf_diff;

            DBG_ASSERT_VALUE(f_diff,           BxDF_PBR_DIFF_F);
            DBG_ASSERT_VALUE(pdf_diff,         BxDF_PBR_Diff_PDF);
        }
    }

    if (FEATURE_ENABLED(GlassMaterials))
    {
        f_opaque *= 1.0f - hitInfo.Mat.TransmissionFactor;
        pdf_opaque *= 1.0f - hitInfo.Mat.TransmissionFactor;
    }

    f += f_opaque;
    pdf += pdf_opaque;
}

#endif