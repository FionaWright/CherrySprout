#ifndef H_PBR_H
#define H_PBR_H

#include "PathTracing/Flags/Internal/MethodsHlsl.hlsli"
#include "PathTracing/Debug/Internal/OutputColorMacros.hlsli"

#include "Utils/Math/Fresnel.hlsli"
#include "PathTracing/Debug/Assert.hlsli"
#include "MicrofacetModels/GetMM.hlsli"
#include "PathTracing/Structs.h"

#include "BxDFs/Lobes/LambertianLobe.hlsli"
#include "BxDFs/Lobes/SpecularLobe.hlsli"
#include "BxDFs/Lobes/TransmissiveLobe.hlsli"

float GetReflectProb(float iorCurrent, float iorNext, float WdV)
{
    if (DEBUG_ENABLED(ForceReflect))
        return 1.0f;
    else if (CheckTIR(iorCurrent, iorNext, abs(WdV)))
        return 1.0f;
    else if (DEBUG_ENABLED(ForceRefract))
        return 0.0f;

    return Fresnel_Dielectric_Unpolarized(iorCurrent, iorNext, saturate(abs(WdV)));
}

float GetSpecularProb(float WdV, float3 F0)
{
    if (DEBUG_ENABLED(ForceSpecular))
        return 1.0f;
    else if (DEBUG_ENABLED(ForceDiffuse))
        return 0.0f;

    float3 F_select = F_Schlick(WdV, F0);

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
    mm.Init(hitInfo.Mat.Roughness, rngInfo, V_s);
    if (FEATURE_ENABLED(Anisotropy))
        mm.InitAniso(hitInfo);

    float3 H_s = mm.Sample(u1, u2);
    float VdH = dot(H_s, V_s);

    float3 L_s = 0.0f;

    bool isTransmission = hitInfo.Mat.TransmissionFactor > 0 && Rand01(rngInfo) <= hitInfo.Mat.TransmissionFactor;

    DBG_ASSERT_APPROX(length(V_s), 1.0f, 0.02f,  UNNORMALIZED_VECTOR);
    DBG_ASSERT_APPROX(length(H_s), 1.0f, 0.02f,  UNNORMALIZED_VECTOR);
    DBG_ASSERT_APPROX(length(wo), 1.0f, 0.02f,   UNNORMALIZED_VECTOR);
    DBG_ASSERT_VALUE(H_s,                        BxDF_PBR_H_S);
    DBG_OUTPUT1(isTransmission,                  BxDF_IsTransmission);
    DBG_OUTPUT3(H_s,                             BxDF_H_s);
    DBG_OUTPUT3(N_s,                             BxDF_N_s);
    DBG_OUTPUT3(V_s,                             BxDF_V_s);
    DBG_OUTPUT1(NdV,                             BxDF_NdV);
    DBG_OUTPUT1(VdH,                             BxDF_VdH);
    DBG_OUTPUT3(F0,                              BxDF_F0);
    DBG_OUTPUT1(u1,                              BxDF_u1);
    DBG_OUTPUT1(u2,                              BxDF_u2);
    DBG_OUTPUT3(hitInfo.SFrame.ToWorld(H_s),     BxDF_H_w);
    DBG_OUTPUT1(mm.m_alpha,                      BxDF_Alpha);
    DBG_OUTPUT1(mm.D(H_s),                       BxDF_D);

    if (FEATURE_ENABLED(Transmission) && isTransmission)
    {
        float iorNCurrent =  hitInfo.IsEntering ? IOR_N_AIR          : hitInfo.Mat.IOR_N;
        float iorNNext =     hitInfo.IsEntering ? hitInfo.Mat.IOR_N  : IOR_N_AIR;

        float reflectProb = GetReflectProb(iorNCurrent, iorNNext, VdH);
        bool isReflect = reflectProb > 0 && Rand01(rngInfo) <= reflectProb;

        DBG_ASSERT_ZERO(hitInfo.Mat.Metallic,         NO_METAL_GLASS);
        DBG_ASSERT_ZERO(hitInfo.Emission,             NO_EMISSIVE_GLASS);
        DBG_OUTPUT1(reflectProb,                      BxDF_ReflectProb);
        DBG_OUTPUT1(isReflect,                        BxDF_IsReflect);
        DBG_OUTPUT1(iorNCurrent,                      BxDF_iorNCurrent);
        DBG_OUTPUT1(iorNNext,                         BxDF_iorNNext);
        DBG_OUTPUT1(iorNCurrent/iorNNext,             BxDF_eta);

        if (isReflect)
        {
            bool isDiracDelta;
            SpecularLobe_Sample(hitInfo, mm, V_s, N_s, H_s, F0, L_s, f, pdf, isDiracDelta);

            pathState.LastRayWasDiracDelta = isDiracDelta;

            pdf *= reflectProb;

            DBG_ASSERT_VALUE(f,         BxDF_PBR_GLASS_SPEC_F);
            DBG_ASSERT_VALUE(pdf,       BxDF_PBR_GLASS_SPEC_PDF);
            DBG_ASSERT_VALUE(L_s,       BxDF_PBR_GLASS_SPEC_L_S);
        }
        else
        {
            TransmissiveLobe_Sample(hitInfo, mm, V_s, N_s, H_s, iorNCurrent, iorNNext, L_s, f, pdf);

            pdf *= 1.0f - reflectProb;

            DBG_ASSERT_VALUE(f,         BxDF_PBR_GLASS_REFRACT_F);
            DBG_ASSERT_VALUE(pdf,       BxDF_PBR_GLASS_REFRACT_PDF);
            DBG_ASSERT_VALUE(L_s,       BxDF_PBR_GLASS_REFRACT_L_S);
        }

        pdf *= hitInfo.Mat.TransmissionFactor;

        wi = hitInfo.SFrame.ToWorld(L_s);
        DBG_ASSERT_APPROX(length(L_s), 1.0f, 0.02f, UNNORMALIZED_VECTOR);
        DBG_ASSERT_APPROX(length(wi), 1.0f, 0.02f, UNNORMALIZED_VECTOR);
        DBG_ASSERT_GE(pdf, 0.0f, NON_POSITIVE_PDF);
        DBG_OUTPUT3(L_s,        BxDF_L_s);
        return;
    }

    float specProb = GetSpecularProb(NdV, F0);
    bool isSpecular = specProb > 0 && Rand01(rngInfo) <= specProb;

    DBG_OUTPUT1(specProb,                      BxDF_SpecProb);
    DBG_OUTPUT1(isSpecular,                    BxDF_IsSpecular);

    if (isSpecular)
    {
        bool isDiracDelta;
        SpecularLobe_Sample(hitInfo, mm, V_s, N_s, H_s, F0, L_s, f, pdf, isDiracDelta);

        pathState.LastRayWasDiracDelta = isDiracDelta;

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

    if (FEATURE_ENABLED(Transmission))
        pdf *= 1.0f - hitInfo.Mat.TransmissionFactor;

    wi = hitInfo.SFrame.ToWorld(L_s);
    DBG_ASSERT_APPROX(length(L_s), 1.0f, 0.02f, UNNORMALIZED_VECTOR);
    DBG_ASSERT_APPROX(length(wi), 1.0f, 0.02f, UNNORMALIZED_VECTOR);
    DBG_ASSERT_GE(pdf, 0.0f, NON_POSITIVE_PDF);
    DBG_OUTPUT3(L_s,        BxDF_L_s);
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
    mm.Init(hitInfo.Mat.Roughness, V_s);
    if (FEATURE_ENABLED(Anisotropy))
        mm.InitAniso(hitInfo);

    float VdH = dot(H_s, V_s);

    DBG_OUTPUT3(N_s,                             Eval_N_s);
    DBG_OUTPUT3(V_s,                             Eval_V_s);
    DBG_OUTPUT3(L_s,                             Eval_L_s);
    DBG_OUTPUT3(wi,                              Eval_L_w);
    DBG_OUTPUT3(H_s,                             Eval_H_s);
    DBG_OUTPUT3(hitInfo.SFrame.ToWorld(H_s),     Eval_H_w);
    DBG_OUTPUT1(NdV,                             Eval_NdV);
    DBG_OUTPUT1(VdH,                             Eval_VdH);
    DBG_OUTPUT3(F0,                              Eval_F0);
    DBG_OUTPUT1(mm.m_alpha,                      Eval_Alpha);
    DBG_OUTPUT1(mm.D(H_s),                       Eval_D);

    f = 0;
    pdf = 0;

    if (FEATURE_ENABLED(Transmission) && hitInfo.Mat.TransmissionFactor > 0)
    {
        float iorNCurrent =  hitInfo.IsEntering ? IOR_N_AIR          : hitInfo.Mat.IOR_N;
        float iorNNext =     hitInfo.IsEntering ? hitInfo.Mat.IOR_N  : IOR_N_AIR;

        float reflectProb = GetReflectProb(iorNCurrent, iorNNext, VdH);

        DBG_OUTPUT1(reflectProb,                      Eval_ReflectProb);
        DBG_ASSERT_ZERO(hitInfo.Mat.Metallic,         NO_METAL_GLASS);
        DBG_ASSERT_ZERO(hitInfo.Emission,             NO_EMISSIVE_GLASS);

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

            DBG_ASSERT_VALUE(f_reflect,           BxDF_PBR_GLASS_SPEC_F);
            DBG_ASSERT_VALUE(pdf_reflect,         BxDF_PBR_GLASS_SPEC_PDF);
            DBG_OUTPUT1(f_reflect,                Eval_f_reflect);
            DBG_OUTPUT1(pdf_reflect,              Eval_PDF_reflect);
        }

        {
            float3 f_refract;
            float pdf_refract;
            TransmissiveLobe_Evaluate(hitInfo, mm, V_s, N_s, H_s, L_s, iorNCurrent, iorNNext, f_refract, pdf_refract);

            pdf_refract *= 1.0f - reflectProb;

            if (DEBUG_ENABLED(ForceReflect))
                f_refract = 0.0f;

            f_trans += f_refract;
            pdf_trans += pdf_refract;

            DBG_ASSERT_VALUE(f_refract,           BxDF_PBR_GLASS_REFRACT_F);
            DBG_ASSERT_VALUE(pdf_refract,         BxDF_PBR_GLASS_REFRACT_PDF);
            DBG_OUTPUT1(f_refract,                Eval_f_refract);
            DBG_OUTPUT1(pdf_refract,              Eval_PDF_refract);
        }

        f_trans *= hitInfo.Mat.TransmissionFactor;
        pdf_trans *= hitInfo.Mat.TransmissionFactor;

        DBG_OUTPUT1(f_trans,                Eval_f_trans);
        DBG_OUTPUT1(pdf_trans,              Eval_PDF_trans);

        f += f_trans;
        pdf += pdf_trans;
    }

    float specProb = GetSpecularProb(NdV, F0);

    DBG_OUTPUT1(specProb,                      Eval_SpecProb);

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
            DBG_OUTPUT1(f_spec,                Eval_f_spec);
            DBG_OUTPUT1(pdf_spec,              Eval_PDF_spec);
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
            DBG_OUTPUT1(f_diff,                Eval_f_diff);
            DBG_OUTPUT1(pdf_diff,              Eval_PDF_diff);
        }
    }

    if (FEATURE_ENABLED(Transmission))
    {
        f_opaque *= 1.0f - hitInfo.Mat.TransmissionFactor;
        pdf_opaque *= 1.0f - hitInfo.Mat.TransmissionFactor;
    }

    f += f_opaque;
    pdf += pdf_opaque;

    DBG_ASSERT_GE(pdf, 0.0f,                NON_POSITIVE_PDF);
    DBG_OUTPUT1(f,                          Eval_f);
    DBG_OUTPUT1(f_opaque,                   Eval_f_opaque);
    DBG_OUTPUT1(pdf,                        Eval_PDF);
    DBG_OUTPUT1(pdf_opaque,                 Eval_PDF_opaque);
}

#endif