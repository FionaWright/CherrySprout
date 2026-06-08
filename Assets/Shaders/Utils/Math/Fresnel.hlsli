#ifndef H_FRESNEL_SPECTRAL_H
#define H_FRESNEL_SPECTRAL_H

#include "Utils/Math/Complex.hlsli"

// Nabla Implementation: (Dielectric only, No Conductor)
// https://github.com/Devsh-Graphics-Programming/Nabla/blob/82726cbac7e18f702dfebdacd862697216e9b0fd/include/nbl/builtin/hlsl/bxdf/fresnel.hlsl

// Theory:
// https://en.wikipedia.org/wiki/Maxwell's_equations
// https://em.geosci.xyz/content/maxwell1_fundamentals/reflection_and_refraction/Fresnel_equations.html
// https://pbr-book.org/4ed/Reflection_Models/Specular_Reflection_and_Transmission
// https://scispace.com/pdf/handbook-of-optical-materials-3e2ukwmx2l.pdf
// https://seblagarde.wordpress.com/2013/04/29/memo-on-fresnel-equations/

void Fresnel_Dielectric_Polarized(float n1, float n2, float cosTi, out float rp2, out float rs2)
{
    float sin2Ti = 1.0f - cosTi * cosTi;

    float eta = n1 / n2;
    float eta2 = eta * eta;

    float sin2Tt = eta2 * sin2Ti;
    float cosTt = sqrt(max(0.0f, 1 - sin2Tt));

    if (sin2Tt >= 1.0f)
    {
        rp2 = rs2 = 1.0f;
        return;
    }

    float rp = (n1*cosTt - n2*cosTi) / max(1e-6f, n1*cosTt + n2*cosTi);
    rp2 = rp * rp;

    float rs = (n1*cosTi - n2*cosTt) / max(1e-6f, n1*cosTi + n2*cosTt);
    rs2 = rs * rs;
}

float Fresnel_Dielectric_Unpolarized(float n1, float n2, float cosTheta)
{
    cosTheta = saturate(cosTheta);

    float rp2, rs2;
    Fresnel_Dielectric_Polarized(n1, n2, cosTheta, rp2, rs2);

    float reflectanceProb = (rp2 + rs2) * 0.5f;
    return reflectanceProb;
}

void Fresnel_Conductor_Polarized(Complex c1, Complex c2, float cosTi, out float rp2, out float rs2)
{
    float sin2Ti = 1.0f - cosTi * cosTi;

    Complex eta = Div(c1, c2);
    Complex eta2 = Mul(eta, eta);

    Complex sin2Tt = Mul(eta2, sin2Ti);
    sin2Tt = Sub(CreateComplex(1.0f, 0.0f), sin2Tt);
    Complex cosTt = Sqrt(sin2Tt);

    Complex t1 = Mul(c1, cosTt);
    Complex t2 = Mul(c2, cosTt);
    Complex i1 = Mul(c1, cosTi);
    Complex i2 = Mul(c2, cosTi);

    Complex t1_a_i2 = Add(t1, i2);
    Complex t1_s_i2 = Sub(t1, i2);
    Complex i1_a_t2 = Add(i1, t2);
    Complex i1_s_t2 = Sub(i1, t2);

    Complex rp = Div(t1_s_i2, t1_a_i2);
    rp2 = rp.Re * rp.Re + rp.Im * rp.Im;

    Complex rs = Div(i1_s_t2, i1_a_t2);
    rs2 = rs.Re * rs.Re + rs.Im * rs.Im;
}

float Fresnel_Conductor_Unpolarized(Complex c1, Complex c2, float cosTheta)
{
    cosTheta = saturate(cosTheta);

    float rp2, rs2;
    Fresnel_Conductor_Polarized(c1, c2, cosTheta, rp2, rs2);

    float reflectanceProb = (rp2 + rs2) * 0.5f;
    return reflectanceProb;
}

#define METALNESS_CONDUCTOR_THRESHOLD 0.5f
bool IsConductor(float metalness) { return metalness > METALNESS_CONDUCTOR_THRESHOLD; }

float Fresnel_Maxwell(Complex c1, Complex c2, float cosTheta, float metallic)
{
    if (IsConductor(metallic))
        return Fresnel_Conductor_Unpolarized(c1, c2, cosTheta);
    else
        return Fresnel_Dielectric_Unpolarized(c1.Re, c2.Re, cosTheta);
}

#endif