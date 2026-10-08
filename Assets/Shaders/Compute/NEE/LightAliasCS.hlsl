#include "Utils/SharedUtils.h"
#include "Utils/CBVs.h"
#include "PathTracing/Structs.h"
#include "PathTracing/NEE/LSD/Alias.h"

#include "Utils/Constants.h"

RWStructuredBuffer<float> gPunctualPDF : register(u0);
RWStructuredBuffer<float> gEmissivePDF  : register(u1);
RWStructuredBuffer<AliasEntry> gAliasTable  : register(u2);

ConstantBuffer<CbvTotalLuminances> gTotalLums : register(b0);

#include "Compute/NEE/Utils.hlsli"

#ifndef MAX_STACK_SIZE
#error Maximum stack size must be passed in through define
#endif

// https://www.keithschwarz.com/darts-dice-coins/
// https://arxiv.org/pdf/2106.12270
// https://en.wikipedia.org/wiki/Alias_method

[numthreads(1,1,1)]
void CSMain(uint3 DTid : SV_DispatchThreadID)
{
    float punctualWeight = gTotalLums.PunctualTotalLuminance;
    float emissiveWeight = gTotalLums.EmissiveTotalLuminance;
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

    // Env Map
    if (envMapCount > 0)
    {
        float pdf = envMapWeight / totalWeight;
        gAliasTable[idx].PDF = pdf;
        gAliasTable[idx].Alias = -1;
        rollingSum += pdf;
        idx++;
    }

    // Punctual Lights
    for (int i = 0; i < punctualsCount; i++)
    {
        float pdf = gPunctualPDF[i] / totalWeight;
        gAliasTable[idx].PDF = pdf;
        gAliasTable[idx].Alias = -1;
        rollingSum += pdf;
        idx++;
    }

    for (int i = 0; i < emissivesCount; i++)
    {
        float pdf = gEmissivePDF[i] / totalWeight;
        gAliasTable[idx].PDF = pdf;
        gAliasTable[idx].Alias = -1;
        rollingSum += pdf;
        idx++;
    }

    uint stackSmall[MAX_STACK_SIZE];
    uint stackSmallPtr = 0;

    uint stackLarge[MAX_STACK_SIZE];
    uint stackLargePtr = 0;

    // Normalize, multiply by N and fill stacks
    for (int i = 0; i < lsdCount; i++)
    {
        gAliasTable[i].PDF /= rollingSum;
        gAliasTable[i].Threshold = gAliasTable[i].PDF * lsdCount;

        if (gAliasTable[i].Threshold < 1.0f)
        {
            stackSmall[stackSmallPtr] = i;
            stackSmallPtr++;
        }
        else
        {
            stackLarge[stackLargePtr] = i;
            stackLargePtr++;
        }
    }

    // Vose's Method
    // TODO: Sorting beforehand can lead to better runtime performance

    while (stackSmallPtr > 0 && stackLargePtr > 0)
    {
        stackSmallPtr--;
        uint idxS = stackSmall[stackSmallPtr];
        float diffS = 1.0f - gAliasTable[idxS].Threshold;

        stackLargePtr--;
        uint idxL = stackLarge[stackLargePtr];

        gAliasTable[idxS].Alias = idxL;
        gAliasTable[idxL].Threshold -= diffS;

        if (gAliasTable[idxL].Threshold < 1.0f) // TODO: See Vose's. Can be made more numerically stable
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
