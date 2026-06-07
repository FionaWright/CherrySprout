#ifndef H_ROOT_CONSTANTS_H
#define H_ROOT_CONSTANTS_H

#include <cstdint>
#include <d3d12.h>

struct RootConstants
{
    void Init(uint32_t regIdx, uint32_t regSpace, uint32_t totalBytes);
    void Bind_Graphics(ID3D12GraphicsCommandList* cmdList, const void* pData, uint32_t offset = 0) const;
    void Bind_Compute(ID3D12GraphicsCommandList* cmdList, const void* pData, uint32_t offset = 0) const;

    uint32_t RegisterIdx = 0;
    uint32_t RegisterSpace = 0;
    uint32_t Num32BitValues = 0;
};

#endif