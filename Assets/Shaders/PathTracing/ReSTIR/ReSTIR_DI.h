#ifndef H_TARGET_H
#define H_TARGET_H

struct CbvRestirSettings
{
    hlsl::uint ConfidenceCap;
    hlsl::uint NumCandidates;
};

struct LightSampleSelection
{
    hlsl::float3 ScaledDirection; // Direction * Distance
    hlsl::uint LightIndex;

    hlsl::float3 Radiance;
    hlsl::float p;
};

typedef ReservoirDI Reservoir<LightSampleSelection>;

LightSampleSelection Generate(inout RngInfo rngInfo, HitInfo hitInfo, float3 wo, float3 hitPosOffset)
{
    BxDF bxdf;

    float3 radiance_light;
    float pdf_light;
    float3 wi;
    float3 distance;
    SampleLight(rngInfo, hitInfo, bxdf, wo, hitPosOffset, radiance_light, pdf_light, wi, distance);

    uint lightIndex = 0; // TODO

    LightSampleSelection X_i;
    X_i.ScaledDirection = normalize(wi) * distance;
    X_i.LightIndex = lightIndex;
    X_i.Radiance = radiance_light;
    return X_i;
}

float Target(LightSampleSelection X_i)
{
    float NdL = abs(dot(hitInfo.Ns_ff, normalize(X_i.ScaledDirection)));

    if (pdf_light < EPSILON || NdL > 0)
        return 0.0f; // Null? 

    BxDF bxdf;
    bxdf.Evaluate(hitInfo, wi, wo, f_bxdf, pdf_bxdf);

    float radiance = f_bxdf * X_i.Radiance * NdL;
    return Luminance(radiance);
}

float PDF(LightSampleSelection X_i)
{
    return max(1e-6f, pdf_light);
}

#endif