#ifndef H_BRDF_SPECULAR_LOBE_H
#define H_BRDF_SPECULAR_LOBE_H

#define ROUGHNESS_DIRAC_THRESHOLD EPSILON

void SpecularLobe_Sample(
        HitInfo hitInfo,       MicrofacetModel mm,
        float3 V_s,             float3 N_s,            float3 H_s,

        float3 F0,

        out float3 L_s,
        out float3 f,
        out float pdf,
        out bool isDiracDelta
    )
{
    L_s = NormalizeSafe(reflect(-V_s, H_s), N_s);

    // Terminate ray if wi ends up inside surface
    if (L_s.z <= 0.0f)
    {
        f = 0.0f;
        pdf = 1.0f;
        isDiracDelta = false;
        DBG_OUTPUT1(0,                F);
        DBG_OUTPUT1(0,                G);
        return;
    }

    float NdV = SSpaceCosTheta(V_s);
    float NdL = SSpaceCosTheta(L_s);
    float VdH = dot(H_s, V_s);

    float3 F = F_Schlick(VdH, F0);
    float D = mm.D(H_s);
    float G = mm.G2(NdL, NdV);
    pdf = mm.PDF(D, H_s, V_s);

    pdf /= (4.0f * max(1e-6, VdH)); // Reflection PDF

    DBG_OUTPUT3(F,                F);
    DBG_OUTPUT1(G,                G);

    // Dirac Delta
    if (hitInfo.Mat.Roughness < ROUGHNESS_DIRAC_THRESHOLD)
    {
        L_s = reflect(-V_s, N_s);
        f = F;
        pdf = 1;
        isDiracDelta = true;
        return;
    }

    float3 specularBrdf = (D * G * F) / max(1e-6, 4 * NdV * NdL);
    f = specularBrdf;

    isDiracDelta = false;
}

void SpecularLobe_Evaluate(
        HitInfo hitInfo,       MicrofacetModel mm,
        float3 V_s,             float3 N_s,            float3 H_s,          float3 L_s,

        float3 F0,

        out float3 f,
        out float pdf
    )
{
    // Terminate ray if wi ends up inside surface
    if (L_s.z <= 0.0f)
    {
        f = 0.0f;
        pdf = 1.0f;
        DBG_OUTPUT1(0,                F);
        DBG_OUTPUT1(0,                G);
        return;
    }

    float NdV = SSpaceCosTheta(V_s);
    float NdL = SSpaceCosTheta(L_s);
    float VdH = dot(H_s, V_s);

    float3 F = F_Schlick(VdH, F0);
    float D = mm.D(H_s);
    float G = mm.G2(NdL, NdV);

    pdf = mm.PDF(D, H_s, V_s);
    pdf /= (4.0f * max(1e-6, VdH)); // Reflection PDF

    DBG_OUTPUT3(F,                Eval_F);
    DBG_OUTPUT1(G,                Eval_G);

    // Dirac Delta
    if (hitInfo.Mat.Roughness < ROUGHNESS_DIRAC_THRESHOLD)
    {
        f = 0;
        pdf = 1;
        return;
    }

    float3 specularBrdf = (D * G * F) / max(1e-6, 4 * NdV * NdL);
    f = specularBrdf;
}

void SpecularLobe_Pdf(
        HitInfo hitInfo,       MicrofacetModel mm,
        float3 V_s,            float3 H_s,          float3 L_s,

        out float pdf
    )
{
    // Terminate ray if wi ends up inside surface
    if (L_s.z <= 0.0f)
    {
        pdf = 1.0f;
        return;
    }

    float VdH = dot(H_s, V_s);

    float D = mm.D(H_s);
    pdf = mm.PDF(D, H_s, V_s);
    pdf /= (4.0f * max(1e-6, VdH)); // Reflection PDF

    // Dirac Delta
    if (hitInfo.Mat.Roughness < ROUGHNESS_DIRAC_THRESHOLD)
    {
        pdf = 1;
        return;
    }
}

#endif