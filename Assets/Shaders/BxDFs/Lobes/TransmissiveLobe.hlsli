#ifndef H_BTDF_TRANSMISSIVE_LOBE_H
#define H_BTDF_TRANSMISSIVE_LOBE_H

// https://www.cs.cornell.edu/~srm/publications/EGSR07-btdf.pdf

void TransmissiveLobe_Sample(
        HitInfo hitInfo,        MicrofacetModel mm,
        float3 V_s,             float3 N_s,             float3 H_s,
        float nCurrent,         float nNext,

        out float3 L_s,
        out float3 f,
        out float pdf
    )
{
    float eta = nCurrent / nNext;
    float eta2 = eta * eta;

    L_s = refract(-V_s, H_s, eta);

    if (all(L_s == 0))
    {
        pdf = 0;
        f = 0;
        return;
    }

    float VdH = dot(H_s, V_s);
    float LdH = dot(L_s, H_s);
    float NdL = SSpaceCosTheta(L_s);
    float NdV = SSpaceCosTheta(V_s);
    float NdH = SSpaceCosTheta(H_s);

    if (LdH * VdH >= 0)
    {
        pdf = 1;
        f = 0;
        return;
    }

    float F = Fresnel_Dielectric_Unpolarized(nCurrent, nNext, abs(VdH));
    float D = mm.D(H_s);
    float G = mm.G2(abs(NdL), abs(NdV));
    float mmPdf = mm.PDF(D, H_s, V_s);

    DBG_OUTPUT1(F,                F);
    DBG_OUTPUT1(G,                G);

    float denom = nCurrent * VdH + nNext * LdH;
    float denom2 = denom * denom;
    float jacobian = nNext * nNext * abs(LdH) / max(1e-6, denom2);

    pdf = jacobian * mmPdf;

    // TODO
    //if (!hitInfo.IsEntering)
    //    f *= exp(-sigmaA * hitInfo.RayT);

    // TODO: Wrong
    if (false && hitInfo.Mat.Roughness < EPSILON)
    {
        f = (1 - F) * eta;
        return;
    }

    f = (1 - F) * D * G * hitInfo.Mat.TransmissionColor;
    f *= (abs(VdH) / abs(NdV)) * (abs(LdH) / abs(NdL));
    f *= eta2;
    f /= max(1e-6, denom2);
}

void TransmissiveLobe_Evaluate(
        HitInfo hitInfo,        MicrofacetModel mm,
        float3 V_s,             float3 N_s,             float3 H_s,         float3 L_s,
        float nCurrent,         float nNext,

        out float3 f,
        out float pdf
    )
{
    float eta = nCurrent / nNext;
    float eta2 = eta * eta;

    if (all(L_s == 0))
    {
        pdf = 0;
        f = 0;
        return;
    }

    float VdH = dot(H_s, V_s);
    float LdH = dot(L_s, H_s);
    float NdL = SSpaceCosTheta(L_s);
    float NdV = SSpaceCosTheta(V_s);
    float NdH = SSpaceCosTheta(H_s);

    if (LdH * VdH >= 0)
    {
        pdf = 1;
        f = 0;
        return;
    }

    float F = Fresnel_Dielectric_Unpolarized(nCurrent, nNext, abs(VdH));
    float D = mm.D(H_s);
    float G = mm.G2(abs(NdL), abs(NdV));
    float mmPdf = mm.PDF(D, H_s, V_s);

    DBG_OUTPUT1(F,                F);
    DBG_OUTPUT1(G,                G);

    float denom = nCurrent * VdH + nNext * LdH;
    float denom2 = denom * denom;
    float jacobian = nNext * nNext * abs(LdH) / max(1e-6, denom2);

    pdf = jacobian * mmPdf;

    // TODO
    //if (!hitInfo.IsEntering)
    //    f *= exp(-sigmaA * hitInfo.RayT);

    // TODO: Wrong
    if (false && hitInfo.Mat.Roughness < EPSILON)
    {
        f = (1 - F) * eta;
        return;
    }

    f = (1 - F) * D * G * hitInfo.Mat.TransmissionColor;
    f *= (abs(VdH) / abs(NdV)) * (abs(LdH) / abs(NdL));
    f *= eta2;
    f /= max(1e-6, denom2);
}

#endif