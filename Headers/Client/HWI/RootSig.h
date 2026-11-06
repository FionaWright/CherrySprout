//
// Created by fionaw on 25/09/2025.
//

#ifndef PT_ROOTSIG_H
#define PT_ROOTSIG_H
#include "RootConstants.h"

struct RangeInfo
{
    D3D12_DESCRIPTOR_RANGE_TYPE Type;
    uint32_t NumDescriptors;
    uint32_t BaseRegister;
    uint32_t RegisterSpace;
    D3D12_DESCRIPTOR_RANGE_FLAGS Flags = D3D12_DESCRIPTOR_RANGE_FLAG_NONE;
};

class RootSig
{
public:
    void Init(ID3D12Device* device, const CD3DX12_ROOT_PARAMETER1* params, UINT paramCount, const D3D12_STATIC_SAMPLER_DESC* pSamplers, UINT samplerCount);
    void AddCbvRange(uint32_t spaceIdx, uint32_t cbvCount);
    void AddSrvRange(uint32_t spaceIdx, uint32_t srvCount);
    void AddUavRange(uint32_t spaceIdx, uint32_t uavCount);
    void AddRootConstants(RootConstants* rootConstants);
    void AddSrvRangeBindless(uint32_t spaceIdx);
    void Init(ID3D12Device* device, const D3D12_STATIC_SAMPLER_DESC* samplers = nullptr, UINT samplerCount = 0);
    void QuickInit(ID3D12Device* device, uint32_t cbvCount, uint32_t srvCount, uint32_t uavCount, const D3D12_STATIC_SAMPLER_DESC* samplers = nullptr, UINT samplerCount = 0);

    ID3D12RootSignature* Get() const { return m_rootSignature.Get(); }

    int GetParamIndexSRV(const uint32_t spaceIdx) const { return m_paramIdxSRV.at(spaceIdx); }
    int GetParamIndexCBV(const uint32_t spaceIdx) const { return m_paramIdxCBV.at(spaceIdx); }
    int GetParamIndexUAV(const uint32_t spaceIdx) const { return m_paramIdxUAV.at(spaceIdx); }
    int GetParamIndexRootConstants() const { return m_paramIdxRootConstants; }
    int GetParamIndexSrvBindless(const uint32_t bindlessIdx) const { return m_paramIdxSrvBindless.at(bindlessIdx); }

private:
    RootConstants* m_rootConstants = nullptr;
    std::vector<RangeInfo> m_rangeInfos;
    uint32_t m_paramCount = 0;

    ComPtr<ID3D12RootSignature> m_rootSignature = nullptr;

    int m_paramIdxRootConstants = -1;
    std::vector<int> m_paramIdxCBV;
    std::vector<int> m_paramIdxSRV;
    std::vector<int> m_paramIdxUAV;
    std::vector<int> m_paramIdxSrvBindless;
};


#endif //PT_ROOTSIG_H