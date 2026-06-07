
#include "System/pch.h"

#include "HWI/RootConstants.h"

#include "Utils/Helper.h"

void RootConstants::Init(const uint32_t regIdx, const uint32_t regSpace, const uint32_t totalBytes)
{
    CherryAssert(totalBytes % 4 == 0);

    RegisterIdx = regIdx;
    RegisterSpace = regSpace;
    Num32BitValues = totalBytes / 4;
}

void RootConstants::Bind_Graphics(ID3D12GraphicsCommandList* cmdList, const void* pData, const uint32_t offset) const
{
    cmdList->SetGraphicsRoot32BitConstants(0, Num32BitValues, pData, offset);
}

void RootConstants::Bind_Compute(ID3D12GraphicsCommandList* cmdList, const void* pData, const uint32_t offset) const
{
    cmdList->SetComputeRoot32BitConstants(0, Num32BitValues, pData, offset);
}
