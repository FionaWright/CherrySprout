#include "Utils/SharedUtils.h"
#include "Utils/CBVs.h"
#include "PathTracing/Structs.h"

#include "Utils/Constants.h"

RWStructuredBuffer<float> gPunctualPDF : register(u0);
RWStructuredBuffer<ProbabilityDistributionSample> gCDF  : register(u1);

ConstantBuffer<CbvTotalLuminances> gTotalLums : register(b0);

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
    uint punctualLightCount;
    uint punctualLightStride;
    gPunctualPDF.GetDimensions(punctualLightCount, punctualLightStride);

    float rollingSum = 0.0f;
    uint idx = 0;

    if (gTotalLums.PunctualTotalLuminance == 0.0f)
        punctualLightCount = 0;

    float delta = 1.0f / (punctualLightCount + 1);

    float averageEnvMapLuminance = gTotalLums.EnvMapTotalLuminance / 10000; // TODO: Figure out better weight
    // Use 50/50 split between env map and others?

    //float totalLuminance = gTotalLums.PunctualTotalLuminance + ENV_MAP_IMPORTANCE_RADIANT_INTENSITY;
    //float totalLuminance = gTotalLums.PunctualTotalLuminance + gTotalLums.EnvMapTotalLuminance;
    float totalLuminance = gTotalLums.PunctualTotalLuminance + averageEnvMapLuminance;

    // Env Map assigned to Idx 0
    //float envMapPDF = ENV_MAP_IMPORTANCE_RADIANT_INTENSITY * delta / totalLuminance;
    //float envMapPDF = gTotalLums.EnvMapTotalLuminance * delta / totalLuminance;
    float envMapPDF = averageEnvMapLuminance * delta / totalLuminance;
    AddCdfPdf(envMapPDF, rollingSum, idx);

    // Punctual Lights
    for (int x = 0; x < punctualLightCount; x++)
    {
        float pdf = gPunctualPDF[x] * delta / totalLuminance;
        AddCdfPdf(pdf, rollingSum, idx);
    }

    // Normalize
    for (int x = 0; x < punctualLightCount + 1; x++)
    {
        gCDF[x].PDF /= rollingSum;
        gCDF[x].CDF /= rollingSum;
    }
}
