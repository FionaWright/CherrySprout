#include "System/pch.h"

#include "Render/LightImportanceSampler.h"

#include "HWI/D3D.h"
#include "Utils/Helper.h"

void LightImportanceSampler::BuildEnvMapDistributions(D3D* d3d, ID3D12GraphicsCommandList* cmdList, Heap* heap, D12Resource* envMap)
{
    if (!m_envMapPmf.IsInitialized())
    {
        initializeResources(d3d, heap, envMap);
    }

    // Get Sum of Luminance Pass
    {
        m_setSumLum.SetSRV_Tex2D(d3d->GetDevice(), 0, envMap, envMap->GetDesc().Format);
        m_setSumLum.SetUAV_Tex2D(d3d->GetDevice(), 0, &m_envMapSumLumBufferRW, m_envMapSumLumBufferRW.GetDesc().Format);

        d3d->Flush();
        const auto cmdListPtr = d3d->GetAvailableCmdList(D3D12_COMMAND_LIST_TYPE_DIRECT);
        const auto cmdList_inner = cmdListPtr.Get();

        m_envMapSumLumBufferRW.Transition(cmdList_inner, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);

        V(cmdList_inner->Close());
        d3d->ExecuteCommandList(cmdList_inner);
        d3d->Flush();
    }

    // PMF Pass
    {

    }

    // CDF Conditional Pass
    {

    }

    // CDF Marginal Pass
    {

    }
}

void LightImportanceSampler::initializeResources(const D3D* d3d, Heap* heap, const D12Resource* envMap)
{
    m_rootSigSrvUav.            SmartInit(d3d->GetDevice(), 0, 1, 1);
    m_rootSigPmf.               SmartInit(d3d->GetDevice(), 1, 1, 1);

    m_setSumLum.                Init(heap);
    m_setPmf.                   Init(heap);
    m_setCdfConditional.        Init(heap);
    m_setCdfMarginal.           Init(heap);

    m_pipelineSumLum.           InitCompute(d3d->GetDevice(), "Compute/MIS/SumLumReductionSearchCS.hlsl", m_rootSigSrvUav.Get());
    m_pipelinePmf.              InitCompute(d3d->GetDevice(), "Compute/MIS/EnvMapPmfCS.hlsl", m_rootSigPmf.Get());
    m_pipelineCdfConditional.   InitCompute(d3d->GetDevice(), "Compute/MIS/EnvMapCdfConditionalCS.hlsl", m_rootSigSrvUav.Get());
    m_pipelineCdfMarginal.      InitCompute(d3d->GetDevice(), "Compute/MIS/EnvMapCdfMarginalCS.hlsl", m_rootSigSrvUav.Get());

    const size_t w = envMap->GetDesc().Width;
    const size_t h = envMap->GetDesc().Height;

     // 9x9 * 32x32 = 288x288
    constexpr float c_blockSize = 288.0f;
    const float fWidth = static_cast<float>(w);
    const float fHeight = static_cast<float>(h);
    const float maxDim = std::max(fWidth, fHeight);
    const size_t numThreadGroups1D = std::ceil(maxDim / c_blockSize);
    const size_t bufferNumElements = numThreadGroups1D * numThreadGroups1D;
    const size_t bufferSize = bufferNumElements * sizeof(float);

    m_envMapSumLumBufferRW.         Init_Buffer("Env Map Sum Luminance Buffer (RW)", d3d->GetDevice(), bufferSize, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);
    m_envMapSumLumBufferReadback.   Init_Buffer("Env Map Sum Luminance Buffer (Readback)", d3d->GetDevice(), bufferSize, D3D12_RESOURCE_FLAG_NONE, true);
    m_envMapPmf.                    Init_Tex2D("Env Map PMF", d3d->GetDevice(), w, h, 1, DXGI_FORMAT_R16_FLOAT, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);
    m_envMapCdfConditional.         Init_Tex2D("Env Map CDF Conditional", d3d->GetDevice(), w, h, 1, DXGI_FORMAT_R16_FLOAT, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);

    D3D12_RESOURCE_DESC desc = {};
    desc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE1D;
    desc.Width = w;
    desc.Height = 1;
    desc.Format = DXGI_FORMAT_R16_FLOAT;
    desc.MipLevels = 1;
    desc.DepthOrArraySize = 1;
    desc.Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
    desc.SampleDesc.Count = 1;
    desc.SampleDesc.Quality = 0;

    m_envMapCdfMarginal.Init("Env Map CDF Marginal", d3d->GetDevice(), desc, D3D12_RESOURCE_STATE_COMMON);
}
