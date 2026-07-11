//
// Created by fionaw on 25/09/2025.
//

#ifndef PT_ROOTSIG_H
#define PT_ROOTSIG_H
#include "RootConstants.h"

class RootSig
{
public:
    void Init(ID3D12Device* device, const CD3DX12_ROOT_PARAMETER1* params, UINT paramCount, const D3D12_STATIC_SAMPLER_DESC* pSamplers, UINT samplerCount);
    void SmartInit(ID3D12Device* device, UINT numCBV, UINT numSRV, UINT numUAV = 0, bool hasSceneTextures = false, const D3D12_STATIC_SAMPLER_DESC* samplers = nullptr,
                   UINT samplerCount = 0, const RootConstants* rootConstants = {});

    ID3D12RootSignature* Get() const { return m_rootSignature.Get(); }

    int GetParamIndexSRV() const { return m_paramIdxSRV; }
    int GetParamIndexCBV() const { return m_paramIdxCBV; }
    int GetParamIndexUAV() const { return m_paramIdxUAV; }
    int GetParamIndexRootConstants() const { return m_paramIdxRootConstants; }
    int GetParamIndexSceneTextures() const { return m_paramIdxSceneTextures; }

private:
    ComPtr<ID3D12RootSignature> m_rootSignature;

    int m_paramIdxSRV = -1;
    int m_paramIdxCBV = -1;
    int m_paramIdxUAV = -1;
    int m_paramIdxRootConstants = -1;
    int m_paramIdxSceneTextures = -1;
};


#endif //PT_ROOTSIG_H