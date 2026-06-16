#ifndef H_PBR_H
#define H_PBR_H

#include "Utils/Math/Fresnel.hlsli"
#include "Utils/Debug/Assert.hlsli"
#include "MicrofacetModels/GetMM.hlsli"

void DiffuseLobe(
    inout RngInfo rngInfo,  HitInfo hitInfo,
    float u1,               float u2,

    out float3 L_s,
    out float3 f,
    out float pdf
)
{
    L_s = RandHemisphereCosineSSpace(u1, u2);

    float NdL = SSpaceCosTheta(L_s);
    pdf = NdL / PI;
    f = hitInfo.Mat.Albedo.rgb * (1.0 - hitInfo.Mat.Metallic);

    DBG_OUTPUT1(0,                F);
    DBG_OUTPUT1(0,                G);
}

void SpecularLobe(
    inout RngInfo rngInfo,  HitInfo hitInfo,       MicrofacetModel mm,
    float3 V_s,             float3 N_s,            float3 H_s,
    float u1,               float u2,

    float3 F0,

    out float3 L_s,
    out float3 f,
    out float pdf
)
{
    L_s = NormalizeSafe(reflect(-V_s, H_s), N_s);

    // Terminate ray if wi ends up inside surface
    if (L_s.z <= 0.0f)
    {
        f = 0.0f;
        pdf = 0.0f;
        DBG_OUTPUT1(0,                F);
        DBG_OUTPUT1(0,                G);
        return;
    }

    float NdV = SSpaceCosTheta(V_s);
    float NdL = SSpaceCosTheta(L_s);
    float NdH = SSpaceCosTheta(H_s);
    float VdH = dot(H_s, V_s);

    float3 F = F_Schlick(VdH, F0);
    float D = mm.D(H_s);
    float G = mm.G2(L_s, V_s);
    pdf = mm.PDF(D, H_s, V_s);

    DBG_OUTPUT3(F,                F);
    DBG_OUTPUT1(G,                G);

    // Dirac Delta
    if (hitInfo.Mat.Roughness < EPSILON)
    {
        L_s = reflect(-V_s, N_s);
        f = F;
        pdf = 1;
        return;
    }

    float3 specularBrdf = (D * G * F) / max(1e-6, 4 * NdV * NdL);
    f = specularBrdf * NdL / max(1e-6, pdf);
}

// https://www.cs.cornell.edu/~srm/publications/EGSR07-btdf.pdf
void TransmissiveLobe(
    inout RngInfo rngInfo,  HitInfo hitInfo,        MicrofacetModel mm,
    float3 V_s,             float3 N_s,             float3 H_s,
    float nCurrent,         float nNext,
    float u1,               float u2,

    out float3 L_s,
    out float3 f,
    out float pdf
)
{
    float eta = nCurrent / nNext;
    float eta2 = eta * eta;

    // Recompute H_s (Used for evaluation ONLY!)
    //H_s = normalize(nCurrent * V_s + nNext * L_s);

    L_s = refract(-V_s, H_s, eta);

    float VdH = dot(H_s, V_s);
    float LdH = dot(L_s, H_s);
    float NdL = SSpaceCosTheta(L_s);
    float NdV = SSpaceCosTheta(V_s);
    float NdH = SSpaceCosTheta(H_s);

    float F = Fresnel_Dielectric_Unpolarized(nCurrent, nNext, abs(VdH));
    float D = mm.D(H_s);
    float G = mm.G2(abs(L_s), V_s);
    float mmPdf = mm.PDF(D, H_s, V_s);

    DBG_OUTPUT1(F,                F);
    DBG_OUTPUT1(G,                G);

    //float denom = nCurrent * VdH + nNext * LdH;
    //float denom2 = denom * denom;
    //float factor = abs(VdH * LdH) / max(1e-6f, denom2) / max(1e-6f, abs(NdL * NdV));
    //pdf = D * NdH * abs(LdH) / max(1e-6f, denom2);

    float denom = LdH + VdH / eta;
    float denom2 = denom * denom;
    float k = abs(VdH) / denom2;

    pdf = k * mmPdf;

    f = (1 - F);

    // TODO
    //if (!hitInfo.IsEntering)
    //    f *= exp(-sigmaA * hitInfo.RayT);

    // TODO
    if (hitInfo.Mat.Roughness < EPSILON)
    {
        f *= eta;
        return;
    }

    // TODO: Explodes. Need more debug systems
    float eval = D * G * abs(LdH  * VdH / max(1e-6, NdV * denom2));

    f *= eta2 * hitInfo.Mat.TransmissionColor;
    //f *= eval * hitInfo.Mat.TransmissionColor;
}

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

        hitInfo.Mat.Metallic = 1.0f;

        DBG_ASSERT_ZERO(hitInfo.Mat.Metallic, NO_METAL_GLASS);
        DBG_ASSERT_ZERO(hitInfo.Li          , NO_EMISSIVE_GLASS);

        float reflectProb;
        bool isReflect = IsReflect(rngInfo, iorCurrent, iorNext, VdH, reflectProb);

        if (isReflect)
        {
            SpecularLobe(rngInfo, hitInfo, mm, V_s, N_s, H_s, u1, u2, F0, L_s, f, pdf);
            pdf *= reflectProb;
            f /= max(1e-6, reflectProb);

            DBG_ASSERT_VALUE(f, BxDF_PBR_GLASS_SPEC_F);
        }
        else
        {
            H_s *= Sign(VdH); // TODO: ?
            TransmissiveLobe(rngInfo, hitInfo, mm, V_s, N_s, H_s, iorCurrent, iorNext, u1, u2, L_s, f, pdf);
            pdf *= 1.0f - reflectProb;
            f /= max(1e-6, 1.0f - reflectProb);
        }

        wi = hitInfo.SFrame.ToWorld(L_s);
        return;
    }

    float specProb;
    bool isSpecular = IsSpecular(rngInfo, VdH, F0, specProb);

    if (isSpecular)
    {
        SpecularLobe(rngInfo, hitInfo, mm, V_s, N_s, H_s, u1, u2, F0, L_s, f, pdf);
        pdf *= specProb;
        f /= max(1e-6, specProb);
    }
    else
    {
        DiffuseLobe(rngInfo, hitInfo, u1, u2, L_s, f, pdf);
        pdf *= 1.0f - specProb;
        f /= max(1e-6, 1.0 - specProb);
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
    // TODO
    f = 0; pdf = 0;
/*
    float3 N_s = float3(0, 0, 1);
    float3 V_s = hitInfo.SFrame.ToLocal(wo);

    float NdV = SSpaceCosTheta(V_s);

    float3 F0 = lerp(float3(0.04, 0.04, 0.04), hitInfo.Mat.Albedo.rgb, hitInfo.Mat.Metallic);

    float u1 = Rand01(rngInfo);
    float u2 = Rand01(rngInfo);

    float3 L_s = 0.0f;

    if (Rand01(rngInfo) < hitInfo.Mat.TransmissionFactor)
    {
        Complex iorMat = CreateComplex(ior, 0.0f);
        Complex iorCurrent = Ternary(hitInfo.Entering, IOR_AIR, iorMat);
        Complex iorNext = Ternary(hitInfo.Entering, iorMat, IOR_AIR);

        DBG_ASSERT(hitInfo.Mat.Metallic < EPSILON, NO_METAL_GLASS);
        DBG_ASSERT(hitInfo.Li           < EPSILON, NO_EMISSIVE_GLASS);

        float reflectProb;
        bool isReflect = IsReflect(rngInfo, iorCurrent, iorNext, NdV, reflectProb);

        if (isReflect)
        {
            SpecularLobe(rngInfo, hitInfo, V_s, N_s, u1, u2, F0, L_s, f, pdf);
            pdf *= reflectProb;
            f /= max(1e-6, reflectProb);
        }
        else
        {
            TransmissiveLobe(rngInfo, hitInfo, V_s, N_s, iorCurrent, iorNext, u1, u2, L_s, f, pdf);
            pdf *= 1.0f - reflectProb;
            f /= max(1e-6, 1.0f - reflectProb);
        }
        return;
    }

    float specProb;
    bool isSpecular = IsSpecular(rngInfo, NdV, F0, specProb);

    if (isSpecular)
    {
        SpecularLobe(rngInfo, hitInfo, V_s, N_s, u1, u2, F0, L_s, f, pdf);
        pdf *= specProb;
        f /= max(1e-6, specProb);
    }
    else
    {
        DiffuseLobe(rngInfo, hitInfo, u1, u2, L_s, f, pdf);
        pdf *= 1.0f - specProb;
        f /= max(1e-6, 1.0 - specProb);
    }
*/
}

#endif