#include "Utils/SharedUtils.h"
#include "Utils/CBVs.h"
#include "PathTracing/Structs.h"
#include "PathTracing/NEE/Alias.h"

#include "Utils/Constants.h"

RWStructuredBuffer<float> gPunctualPDF : register(u0);
RWStructuredBuffer<AliasEntry> gAliasTable  : register(u1);

ConstantBuffer<CbvTotalLuminances> gTotalLums : register(b0);

#ifndef MAX_STACK_SIZE
#error Maximum stack size must be passed in through define
#endif

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

    float averageEnvMapLuminance = gTotalLums.EnvMapTotalLuminance / 10000; // TODO: Figure out better weight
    // Use 50/50 split between env map and others?

    float totalLuminance = gTotalLums.PunctualTotalLuminance + averageEnvMapLuminance;

    // Env Map assigned to Idx 0
    float envMapPDF = averageEnvMapLuminance / totalLuminance;
    gAliasTable[idx].PDF = envMapPDF;
    gAliasTable[idx].Alias = -1;
    rollingSum += envMapPDF;
    idx++;

    // Punctual Lights
    for (int x = 0; x < punctualLightCount; x++)
    {
        float pdf = gPunctualPDF[x] / totalLuminance;
        gAliasTable[idx].PDF = pdf;
        gAliasTable[idx].Alias = -1;
        rollingSum += pdf;
        idx++;
    }

    uint lightCount = punctualLightCount + 1;

    uint stackSmall[MAX_STACK_SIZE];
    uint stackSmallPtr = 0;

    uint stackLarge[MAX_STACK_SIZE];
    uint stackLargePtr = 0;

    // Normalize, multiply by N and fill stacks
    for (int x = 0; x < lightCount; x++)
    {
        gAliasTable[x].PDF /= rollingSum;
        gAliasTable[x].Threshold = gAliasTable[x].PDF * lightCount;

        if (gAliasTable[x].Threshold < 1.0f)
        {
            stackSmall[stackSmallPtr] = x;
            stackSmallPtr++;
        }
        else
        {
            stackLarge[stackLargePtr] = x;
            stackLargePtr++;
        }
    }

    // Naive Method, can be optimized

    while (stackSmallPtr > 0 && stackLargePtr > 0)
    {
        stackSmallPtr--;
        uint idxS = stackSmall[stackSmallPtr];
        float diffS = 1.0f - gAliasTable[idxS].Threshold;

        stackLargePtr--;
        uint idxL = stackLarge[stackLargePtr];

        gAliasTable[idxS].Alias = idxL;
        gAliasTable[idxL].Threshold -= diffS;

        if (gAliasTable[idxL].Threshold < 1.0f)
        {
            stackSmall[stackSmallPtr] = idxL;
            stackSmallPtr++;
        }
        else
        {
            stackLarge[stackLargePtr] = idxL;
            stackLargePtr++;
        }
    }

    while (stackLargePtr > 0)
    {
        stackLargePtr--;
        uint idxL = stackLarge[stackLargePtr];

        gAliasTable[idxL].Threshold = 1.0f;
        gAliasTable[idxL].Alias = idxL;
    }

    while (stackSmallPtr > 0)
    {
        stackSmallPtr--;
        uint idxS = stackSmall[stackSmallPtr];

        gAliasTable[idxS].Threshold = 1.0f;
        gAliasTable[idxS].Alias = idxS;
    }
}
