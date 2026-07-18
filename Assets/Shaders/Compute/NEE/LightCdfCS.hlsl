#include "Utils/SharedUtils.h"
#include "Utils/CBVs.h"
#include "PathTracing/Structs.h"

#include "Utils/Constants.h"

RWStructuredBuffer<float> gPunctualPMF : register(u0);
RWStructuredBuffer<ProbabilityDistributionSample> gCDF  : register(u1);

ConstantBuffer<CbvTotalLuminances> gTotalLums : register(b0);

void AddCdfPmf(float pmf, inout float rollingSum, inout uint idx)
{
    rollingSum += pmf;
    gCDF[idx].PMF = pmf;
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
    gPunctualPMF.GetDimensions(punctualLightCount, punctualLightStride);

    float rollingSum = 0.0f;
    uint idx = 0;

    if (gTotalLums.PunctualTotalLuminance == 0.0f)
        punctualLightCount = 0;

    float delta = 1.0f / (punctualLightCount + 1);

    float averageEnvMapLuminance = gTotalLums.EnvMapTotalLuminance / 10000; // TODO: Figure out better weight

    //float totalLuminance = gTotalLums.PunctualTotalLuminance + ENV_MAP_IMPORTANCE_RADIANT_INTENSITY;
    //float totalLuminance = gTotalLums.PunctualTotalLuminance + gTotalLums.EnvMapTotalLuminance;
    float totalLuminance = gTotalLums.PunctualTotalLuminance + averageEnvMapLuminance;

    // Env Map assigned to Idx 0
    //float envMapPMF = ENV_MAP_IMPORTANCE_RADIANT_INTENSITY * delta / totalLuminance;
    //float envMapPMF = gTotalLums.EnvMapTotalLuminance * delta / totalLuminance;
    float envMapPMF = averageEnvMapLuminance * delta / totalLuminance;
    AddCdfPmf(envMapPMF, rollingSum, idx);

    // Punctual Lights
    for (int x = 0; x < punctualLightCount; x++)
    {
        float pmf = gPunctualPMF[x] * delta / totalLuminance;
        AddCdfPmf(pmf, rollingSum, idx);
    }

    // Normalize
    for (int x = 0; x < punctualLightCount + 1; x++)
    {
        gCDF[x].PMF /= rollingSum;
        gCDF[x].CDF /= rollingSum;
    }
}
