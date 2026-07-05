#include "Utils/SharedUtils.h"
#include "Utils/CBVs.h"

#include "Utils/Constants.h"

RWStructuredBuffer<float> gPunctualPMF : register(u0);
RWTexture1D<float> gCDF  : register(u1);

ConstantBuffer<CbvTotalLuminances> gTotalLums : register(b0);

[numthreads(1,1,1)]
void CSMain(uint3 DTid : SV_DispatchThreadID)
{
    uint punctualLightCount;
    uint punctualLightStride;
    gPunctualPMF.GetDimensions(punctualLightCount, punctualLightStride);

    float rollingSum = 0.0f;
    uint cdfIdx = 0;

    // pdf_env = ENV_MAP_IMPORTANCE_RADIANT_INTENSITY * SamplePDF(envMap);
    // pdf_light = PunctualTotalLuminance / (PunctualTotalLuminance + ENV_MAP_IMPORTANCE_RADIANT_INTENSITY) * SamplePDF(light);

    // Env Map assigned to Idx 0
    //rollingSum += gTotalLums.EnvMapTotalLuminance;
    rollingSum += ENV_MAP_IMPORTANCE_RADIANT_INTENSITY;
    gCDF[cdfIdx] = rollingSum;
    cdfIdx++;

    for (int x = 0; x < punctualLightCount; x++)
    {
        rollingSum += gPunctualPMF[x];
        gCDF[cdfIdx] = rollingSum;
        cdfIdx++;

        gPunctualPMF[x] /= gTotalLums.PunctualTotalLuminance; // TODO: Does the PMF need to be normalized including the env map total lum? If so then env map PMF needs to change too
    }
}
