#include "Utils/SharedUtils.h"
#include "Utils/CBVs.h"
#include "PathTracing/Structs.h"

#include "Utils/Constants.h"

RWStructuredBuffer<float> gPunctualPDF : register(u0);
RWStructuredBuffer<float> gEmissivePDF  : register(u1);
RWStructuredBuffer<ProbabilityDistributionSample> gCDF  : register(u2);

ConstantBuffer<CbvLSD> gCBV : register(b0);

#include "Compute/NEE/Utils.hlsli"

void AddCdfPdf(float pdf, inout float rollingSum, inout uint idx)
{
    rollingSum += pdf;
    gCDF[idx].PDF = pdf;
    gCDF[idx].CDF = rollingSum;
    idx++;
}

// pdf_env = ENV_MAP_IMPORTANCE_RADIANT_INTENSITY * SamplePDF(envMap);
// pdf_light = PunctualTotalLuminance / (PunctualTotalLuminance + ENV_MAP_IMPORTANCE_RADIANT_INTENSITY) * SamplePDF(light);

[numthreads(1,1,1)]
void CSMain(uint3 DTid : SV_DispatchThreadID)
{
    float punctualWeight = gCBV.PunctualTotalLuminance;
    float emissiveWeight = gCBV.EmissiveTotalLuminance;
    float envMapWeight = GetEnvMapWeight(punctualWeight, emissiveWeight);
    float totalWeight = punctualWeight + emissiveWeight + envMapWeight;

    uint envMapCount = GetEnvMapCount();
    uint punctualsCount = GetPunctualsCount();
    uint emissivesCount = GetEmissivesCount();
    uint lsdCount = envMapCount + punctualsCount + emissivesCount;

    if (lsdCount == 0 || totalWeight <= 0.0f)
        return;

    float rollingSum = 0.0f;
    uint idx = 0;

    if (envMapCount > 0)
    {
        float envMapPDF = envMapWeight / totalWeight;
        AddCdfPdf(envMapPDF, rollingSum, idx);
    }

    // Punctual Lights
    for (int i = 0; i < punctualsCount; i++)
    {
        float pdf = gPunctualPDF[i] / totalWeight;
        AddCdfPdf(pdf, rollingSum, idx);
    }

    // Emissive Lights
    for (int i = 0; i < emissivesCount; i++)
    {
        float pdf = gEmissivePDF[i] / totalWeight;
        AddCdfPdf(pdf, rollingSum, idx);
    }

    // Normalize
    for (int i = 0; i < lsdCount; i++)
    {
        gCDF[i].PDF /= rollingSum;
        gCDF[i].CDF /= rollingSum;
    }
}
