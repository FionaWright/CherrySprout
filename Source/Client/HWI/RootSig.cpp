//
// Created by fionaw on 25/09/2025.
//

#include "System/pch.h"
#include "HWI/RootSig.h"
#include "Utils/Helper.h"

void RootSig::Init(ID3D12Device* device, const CD3DX12_ROOT_PARAMETER1* params, const UINT paramCount, const D3D12_STATIC_SAMPLER_DESC* pSamplers, const UINT samplerCount)
{
    D3D12_FEATURE_DATA_ROOT_SIGNATURE featureData = {};
    featureData.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_1;
    HRESULT hr = device->CheckFeatureSupport(D3D12_FEATURE_ROOT_SIGNATURE, &featureData, sizeof(featureData));
    if (FAILED(hr))
    {
        featureData.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_0;
    }

    // Allow input layout and deny unnecessary access to certain pipeline stages.
    constexpr D3D12_ROOT_SIGNATURE_FLAGS rootSignatureFlags =
        D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT |
        D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS |
        D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS;

    CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC rootSignatureDescription;
    rootSignatureDescription.Init_1_1(paramCount, params, samplerCount, pSamplers, rootSignatureFlags);

    ComPtr<ID3DBlob> rootSignatureBlob;
    ComPtr<ID3DBlob> errorBlob;
    hr = D3DX12SerializeVersionedRootSignature(&rootSignatureDescription, featureData.HighestVersion, &rootSignatureBlob, &errorBlob);
    if (FAILED(hr))
    {
        if (errorBlob)
        {
            std::string errorMsg = static_cast<char*>(errorBlob->GetBufferPointer());
            CherryPrint(errorMsg.c_str());
        }
        throw std::exception();
    }

    ComPtr<ID3D12RootSignature> rootSig;
    V(device->CreateRootSignature(0, rootSignatureBlob->GetBufferPointer(), rootSignatureBlob->GetBufferSize(), IID_PPV_ARGS(&rootSig)));
    V(rootSig->SetName(L"Root Signature"));

    m_rootSignature = rootSig;
}

void RootSig::AddCbvRange(const uint32_t spaceIdx, const uint32_t cbvCount)
{
    CherryAssert(cbvCount > 0);
    CherryAssert(m_paramIdxSceneTextures == -1);
    CherryAssert(m_paramIdxSRV.size() == 0);
    CherryAssert(m_paramIdxUAV.size() == 0);
    CherryAssert(spaceIdx == m_paramIdxCBV.size());
    m_paramIdxCBV.emplace_back(m_paramCount);

    const uint32_t baseRegister = m_rootConstants ? m_rootConstants->RegisterIdx + 1 : 0;
    m_rangeInfos.emplace_back(RangeInfo{D3D12_DESCRIPTOR_RANGE_TYPE_CBV, cbvCount, baseRegister, spaceIdx});

    m_paramCount++;
}

void RootSig::AddSrvRange(const uint32_t spaceIdx, const uint32_t srvCount)
{
    CherryAssert(srvCount > 0);
    CherryAssert(m_paramIdxSceneTextures == -1);
    CherryAssert(m_paramIdxUAV.size() == 0);
    CherryAssert(spaceIdx == m_paramIdxSRV.size());
    m_paramIdxSRV.emplace_back(m_paramCount);

    m_rangeInfos.emplace_back(RangeInfo{D3D12_DESCRIPTOR_RANGE_TYPE_SRV, srvCount, 0, spaceIdx});
    m_paramCount++;
}

void RootSig::AddUavRange(const uint32_t spaceIdx, const uint32_t uavCount)
{
    CherryAssert(uavCount > 0);
    CherryAssert(m_paramIdxSceneTextures == -1);
    CherryAssert(spaceIdx == m_paramIdxUAV.size());
    m_paramIdxUAV.emplace_back(m_paramCount);

    m_rangeInfos.emplace_back(RangeInfo{D3D12_DESCRIPTOR_RANGE_TYPE_UAV, uavCount, 0, spaceIdx});
    m_paramCount++;
}

void RootSig::AddRootConstants(RootConstants* rootConstants)
{
    CherryAssert(m_paramIdxSceneTextures == -1);
    CherryAssert(m_paramIdxCBV.size() == 0);
    CherryAssert(m_paramIdxSRV.size() == 0);
    CherryAssert(m_paramIdxUAV.size() == 0);
    CherryAssert(!m_rootConstants && rootConstants);
    m_rootConstants = rootConstants;
    m_paramIdxRootConstants = m_paramCount;
    m_paramCount++;
}

void RootSig::AddSceneTextures(const uint32_t spaceIdx)
{
    m_paramIdxSceneTextures = m_paramCount;

    m_rangeInfos.emplace_back(RangeInfo{D3D12_DESCRIPTOR_RANGE_TYPE_SRV, UINT_MAX, 0, spaceIdx, D3D12_DESCRIPTOR_RANGE_FLAG_DESCRIPTORS_VOLATILE});
    m_paramCount++;
}

void RootSig::Init(ID3D12Device* device, const D3D12_STATIC_SAMPLER_DESC* samplers, const UINT samplerCount)
{
    std::vector<CD3DX12_DESCRIPTOR_RANGE1> ranges;
    std::vector<CD3DX12_ROOT_PARAMETER1> params;

    ranges.reserve(m_rangeInfos.size());

    if (m_rootConstants)
    {
        CD3DX12_ROOT_PARAMETER1 param;
        param.ParameterType = D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;
        param.Constants.ShaderRegister = m_rootConstants->RegisterIdx;
        param.Constants.RegisterSpace = m_rootConstants->RegisterSpace;
        param.Constants.Num32BitValues = m_rootConstants->Num32BitValues;
        param.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
        params.emplace_back(param);
    }

    for (uint32_t i = 0; i < m_rangeInfos.size(); i++)
    {
        ranges.emplace_back();
        ranges.back().Init(m_rangeInfos[i].Type, m_rangeInfos[i].NumDescriptors, m_rangeInfos[i].BaseRegister, m_rangeInfos[i].RegisterSpace, m_rangeInfos[i].Flags);

        CD3DX12_ROOT_PARAMETER1 param;
        param.InitAsDescriptorTable(1, &ranges.back(), D3D12_SHADER_VISIBILITY_ALL);
        params.emplace_back(param);
    }

    Init(device, params.data(), params.size(), samplers, samplerCount);
}

void RootSig::QuickInit(ID3D12Device* device, const uint32_t cbvCount, const uint32_t srvCount, const uint32_t uavCount, const D3D12_STATIC_SAMPLER_DESC* samplers, const UINT samplerCount)
{
    if (cbvCount > 0)
        AddCbvRange(0, cbvCount);

    if (srvCount > 0)
        AddSrvRange(0, srvCount);

    if (uavCount > 0)
        AddUavRange(0, uavCount);

    Init(device, samplers, samplerCount);
}
