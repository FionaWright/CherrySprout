#include "Utils/SharedUtils.h"
#include "Utils/CBVs.h"
#include "PathTracing/Structs.h"
#include "PathTracing/NEE/LSD/Alias.h"

#include "Utils/Constants.h"

RWStructuredBuffer<float> gPunctualPDF : register(u0);
RWStructuredBuffer<float> gEmissivePDF  : register(u1);
RWStructuredBuffer<AliasEntry> gAliasTable  : register(u2);

ConstantBuffer<CbvLSD> gCBV : register(b0);

#include "Compute/NEE/Utils.hlsli"

#ifndef MAX_STACK_SIZE
#error Maximum stack size must be passed in through define
#endif

struct Stack
{
    uint Ptr;
    uint InternalStack[MAX_STACK_SIZE];

    void Push(uint i)
    {
        if (Ptr == MAX_STACK_SIZE)
            return;

        InternalStack[Ptr] = i;
        Ptr++;
    }

    uint Pop()
    {
        if (Ptr == 0)
            return UINT_MAX;

        Ptr--;
        return InternalStack[Ptr];
    }

    bool IsEmpty()
    {
        return Ptr == 0;
    }
};

Stack CreateStack()
{
    Stack s;
    s.Ptr = 0;
    return s;
}

// https://www.keithschwarz.com/darts-dice-coins/
// https://arxiv.org/pdf/2106.12270
// https://en.wikipedia.org/wiki/Alias_method

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

    Stack stackSmall = CreateStack();
    Stack stackLarge = CreateStack();

    // Normalize, multiply by N and fill stacks
    for (int i = 0; i < lsdCount; i++)
    {
        gAliasTable[i].PDF /= rollingSum;
        gAliasTable[i].Threshold = gAliasTable[i].PDF * lsdCount;

        if (gAliasTable[i].Threshold < 1.0f)
            stackSmall.Push(i);
        else
            stackLarge.Push(i);
    }

    // Vose's Method
    // TODO: Sorting beforehand can lead to better runtime performance

    while (!stackSmall.IsEmpty() && !stackLarge.IsEmpty())
    {
        uint idxS = stackSmall.Pop();
        uint idxL = stackLarge.Pop();

        float diffS = 1.0f - gAliasTable[idxS].Threshold;

        gAliasTable[idxS].Alias = idxL;
        gAliasTable[idxL].Threshold -= diffS;

        if (gAliasTable[idxL].Threshold < 1.0f) // TODO: See Vose's. Can be made more numerically stable
            stackSmall.Push(idxL);
        else
            stackLarge.Push(idxL);
    }

    while (!stackLarge.IsEmpty())
    {
        uint idxL = stackLarge.Pop();

        gAliasTable[idxL].Threshold = 1.0f;
        gAliasTable[idxL].Alias = idxL;
    }

    while (!stackSmall.IsEmpty())
    {
        uint idxS = stackSmall.Pop();

        gAliasTable[idxS].Threshold = 1.0f;
        gAliasTable[idxS].Alias = idxS;
    }
}
