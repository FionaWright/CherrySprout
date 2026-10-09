#ifndef H_EVALUATE_HIT_EMISSION_H
#define H_EVALUATE_HIT_EMISSION_H

#include "PathTracing/NEE/SampleEmissive.hlsli"
#include "PathTracing/NEE/LSD/SampleLSD.hlsli"
#include "PathTracing/MIS.hlsli"

float3 EvaluateHitEmission(HitInfo hitInfo, PathState pathState, float3 wo)
{
    if (!FEATURE_ENABLED(Emission))
        return 0.0f;

    float3 L_emission = hitInfo.Emission;
    DBG_SCALE_INTENSITY_EMISSION(L_emission);

    if (!FEATURE_ENABLED(NEE))
        return L_emission;

    bool emissivesInLSD = gSettings.LsdCount != gSettings.LsdBaseEmissives;
    if (!emissivesInLSD || all(L_emission <= 0.0f))
        return L_emission;

    if (pathState.IsPrimaryRay || pathState.LastRayWasDiracDelta)
        return L_emission;

    int emissiveIdx = gInstanceToEmissiveInstanceMap[hitInfo.InstanceIdx];
    DBG_ASSERT_NEQ(emissiveIdx, -1, EMISSIVE_NOT_MAPPED);

    uint lightIdx = emissiveIdx + gSettings.LsdBaseEmissives;

    float pdf_lsd;
    EvaluateLSD(lightIdx, pdf_lsd);

    float pdf_emissive = pdf_lsd * EvaluateEmissivePdf(hitInfo.PrimitiveCount, hitInfo.TriangleArea, hitInfo.Ng_ff, wo, hitInfo.RayT);
    float m = PowerHeuristic(pathState.LastBxdfPdf, pdf_emissive, 1, gSettings.DirectNumSamples);
    L_emission *= m;

    return L_emission;
}

#endif