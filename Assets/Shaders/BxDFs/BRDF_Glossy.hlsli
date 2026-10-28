#ifndef H_BXDF_GLOSSY_H
#define H_BXDF_GLOSSY_H

#include "PathTracing/FeatFlags/FeatFlags.hlsli"
#include "PathTracing/Debug/Internal/OutputColorMacros.hlsli"

#include "Utils/Math/Fresnel.hlsli"
#include "PathTracing/Debug/Assert.hlsli"
#include "MicrofacetModels/GetMM.hlsli"
#include "PathTracing/Structs.h"

#include "BxDFs/Lobes/LambertianDisneyLobe.hlsli"
#include "BxDFs/Lobes/SpecularLobe.hlsli"

BxdfSample BxDF::Sample(
    RngValuesAncillary::RngValuesBxDF rng,
    HitInfo hitInfo,
    float3 wo,

    out BxdfEval eval)
{
    BxdfSample sample;

    float3 N_s = float3(0, 0, 1);
    float3 V_s = hitInfo.SFrame.ToLocal(wo);

    float3 F0 = lerp(float3(0.04, 0.04, 0.04), hitInfo.Mat.Albedo.rgb, hitInfo.Mat.Metallic);

    MicrofacetModel mm;
    mm.Init(hitInfo.Mat.Roughness, V_s);
    if (FEAT_CORE(Anisotropy))
        mm.InitAniso(hitInfo);

    float3 H_s = mm.Sample(rng.Microfacet);

    float3 L_s = 0.0f;

    SpecularLobe_Sample(hitInfo, mm, V_s, N_s, H_s, F0, L_s, eval.Radiance, eval.PDF, sample.IsDirac);

    sample.Wi = hitInfo.SFrame.ToWorld(L_s);
    sample.Lobe = LobeTag::eSpecular;

    {
        DBG_ASSERT_APPROX(length(V_s), 1.0f, 0.02f,  UNNORMALIZED_VECTOR);
        DBG_ASSERT_APPROX(length(H_s), 1.0f, 0.02f,  UNNORMALIZED_VECTOR);
        DBG_ASSERT_APPROX(length(wo), 1.0f, 0.02f,   UNNORMALIZED_VECTOR);
        DBG_ASSERT_VALUE(H_s,                        BxDF_PBR_H_S);
        DBG_OUTPUT3(H_s,                             BxDF_H_s);
        DBG_OUTPUT3(N_s,                             BxDF_N_s);
        DBG_OUTPUT3(V_s,                             BxDF_V_s);
        DBG_OUTPUT3(F0,                              BxDF_F0);
        DBG_OUTPUT1(rng.Microfacet.x,                BxDF_u1);
        DBG_OUTPUT1(rng.Microfacet.y,                BxDF_u2);
        DBG_OUTPUT3(hitInfo.SFrame.ToWorld(H_s),     BxDF_H_w);
        DBG_OUTPUT1(mm.m_alpha,                      BxDF_Alpha);
        DBG_OUTPUT1(mm.D(H_s),                       BxDF_D);
        DBG_ASSERT_VALUE(eval.Radiance,    BxDF_PBR_SPEC_F);
        DBG_ASSERT_VALUE(eval.PDF,         BxDF_PBR_SPEC_PDF);
        DBG_ASSERT_VALUE(L_s,              BxDF_PBR_SPEC_L_S);
        DBG_ASSERT_APPROX(length(L_s), 1.0f, 0.02f, UNNORMALIZED_VECTOR);
        DBG_ASSERT_APPROX(length(sample.Wi), 1.0f, 0.02f, UNNORMALIZED_VECTOR);
        DBG_ASSERT_GE(eval.PDF, 0.0f, NON_POSITIVE_PDF);
        DBG_OUTPUT3(L_s, BxDF_L_s);
    }

    return sample;
}

BxdfEval BxDF::Evaluate(
    HitInfo hitInfo,
    float3 wo,
    float3 wi,
    LobeTag lobe)
{
    BxdfEval eval;

    if (!CoversLobe(lobe, LobeTag::eSpecular))
    {
        eval.Radiance = 0.0f;
        eval.PDF = 0.0f;
        return eval;
    }

    float3 N_s = float3(0, 0, 1);
    float3 V_s = hitInfo.SFrame.ToLocal(wo);
    float3 L_s = hitInfo.SFrame.ToLocal(wi);
    float3 H_s = normalize(V_s + L_s);

    float3 F0 = lerp(float3(0.04, 0.04, 0.04), hitInfo.Mat.Albedo.rgb, hitInfo.Mat.Metallic);

    MicrofacetModel mm;
    mm.Init(hitInfo.Mat.Roughness, V_s);
    if (FEAT_CORE(Anisotropy))
        mm.InitAniso(hitInfo);

    SpecularLobe_Evaluate(hitInfo, mm, V_s, N_s, H_s, L_s, F0, eval.Radiance, eval.PDF);

    {
        DBG_OUTPUT3(N_s,                             Eval_N_s);
        DBG_OUTPUT3(V_s,                             Eval_V_s);
        DBG_OUTPUT3(L_s,                             Eval_L_s);
        DBG_OUTPUT3(wi,                              Eval_L_w);
        DBG_OUTPUT3(H_s,                             Eval_H_s);
        DBG_OUTPUT3(hitInfo.SFrame.ToWorld(H_s),     Eval_H_w);
        DBG_OUTPUT3(F0,                              Eval_F0);
        DBG_OUTPUT1(mm.m_alpha,                      Eval_Alpha);
        DBG_OUTPUT1(mm.D(H_s),                       Eval_D);
        DBG_ASSERT_VALUE(eval.Radiance,           BxDF_PBR_SPEC_F);
        DBG_ASSERT_VALUE(eval.PDF,         BxDF_PBR_SPEC_PDF);
        DBG_OUTPUT1(eval.Radiance,                Eval_f_spec);
        DBG_OUTPUT1(eval.PDF,              Eval_PDF_spec);
        DBG_ASSERT_GE(eval.PDF, 0.0f,           NON_POSITIVE_PDF);
        DBG_OUTPUT3(eval.Radiance,              Eval_f);
        DBG_OUTPUT3(eval.Radiance,                   Eval_f_opaque);
        DBG_OUTPUT1(eval.PDF,                   Eval_PDF);
        DBG_OUTPUT1(eval.PDF,                 Eval_PDF_opaque);
    }

    return eval;
}

uint BxDF::MicrofacetModelRngCount()
{
    MicrofacetModel mm;
    return mm.GetRngCount();
}

uint BxDF::LobeSelectRngCount()
{
    return 0;
}

#endif