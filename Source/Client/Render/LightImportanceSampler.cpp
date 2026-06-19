#include "System/pch.h"

#include "Render/LightImportanceSampler.h"

#include "Debug/GPUEventScoped.h"
#include "HWI/D3D.h"
#include "HWI/Heap.h"
#include "HWI/UploadHeap.h"
#include "Utils/D3DUtils.h"
#include "Utils/Helper.h"

void LightImportanceSampler::BuildEnvMapDistributions(D3D* d3d, Heap* heap, D12Resource* envMap)
{
    if (!m_envMapPmf.IsInitialized())
    {
        initializeResources(d3d, heap, envMap);
    }

    // Get Sum of Luminance Pass
    float totalLuminance = 0;
    {
        m_setSumLum.SetSRV_Tex2D(d3d->GetDevice(), 0, envMap, envMap->GetDesc().Format);
        m_setSumLum.SetUAV_Buffer(d3d->GetDevice(), 0, &m_envMapSumLumBufferRW, m_sumLumBufferNumElements, sizeof(float));

        d3d->Flush();
        {
            const auto cmdListPtr = d3d->GetAvailableCmdList(D3D12_COMMAND_LIST_TYPE_DIRECT);
            const auto cmdList = cmdListPtr.Get();

            {
                GPU_SCOPE(cmdList, "CDF: EnvMap Sum of Luminance Reduction Search");

                m_envMapSumLumBufferRW.Transition(cmdList, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);

                heap->Bind(cmdList);
                cmdList->SetComputeRootSignature(m_rootSigSrvUav.Get());
                cmdList->SetPipelineState(m_pipelineSumLum.GetPSO());
                m_setSumLum.TransitionAllSRVToShaderResource(cmdList);
                m_setSumLum.SetDescriptorTables_Compute(cmdList);

                cmdList->Dispatch(m_sumLumGroupsX, m_sumLumGroupsY, 1);

                m_envMapSumLumBufferRW.Transition(cmdList, D3D12_RESOURCE_STATE_COPY_SOURCE);
                m_envMapSumLumBufferReadback.Transition(cmdList, D3D12_RESOURCE_STATE_COPY_DEST);
                cmdList->CopyBufferRegion(m_envMapSumLumBufferReadback.GetResource(), 0, m_envMapSumLumBufferRW.GetResource(), 0, m_sumLumBufferSize);
            }

            V(cmdList->Close());
            d3d->ExecuteCommandList(cmdList);
        }
        d3d->Flush();

        std::vector<float> buff(m_sumLumBufferNumElements);
        m_envMapSumLumBufferReadback.Readback(buff.data());

        for (int i = 0; i < m_sumLumBufferNumElements; i++)
        {
            totalLuminance += buff[i];
        }
    }

    // PMF & CDF Passes
    {
        UploadHeap uploadHeap;
        uploadHeap.Init(d3d->GetDevice(), Align(sizeof(float), 256));

        const auto cmdListPtr = d3d->GetAvailableCmdList(D3D12_COMMAND_LIST_TYPE_DIRECT);
        const auto cmdList = cmdListPtr.Get();

        // PMF Pass
        {
            GPU_SCOPE(cmdList, "EnvMap PMF");

            m_setPmf.AddCBV(d3d->GetDevice(), sizeof(float), &uploadHeap);
            m_setPmf.SetSRV_Tex2D(d3d->GetDevice(), 0, envMap, envMap->GetDesc().Format);
            m_setPmf.SetUAV_Tex2D(d3d->GetDevice(), 0, &m_envMapPmf, m_envMapPmf.GetDesc().Format);

            m_setPmf.UpdateCBV(0, &totalLuminance);
            m_envMapPmf.Transition(cmdList, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);

            heap->Bind(cmdList);
            cmdList->SetComputeRootSignature(m_rootSigPmf.Get());
            cmdList->SetPipelineState(m_pipelinePmf.GetPSO());
            m_setPmf.TransitionAllSRVToShaderResource(cmdList);
            m_setPmf.SetDescriptorTables_Compute(cmdList);

            DispatchOverTexture(cmdList, 16, envMap->GetDesc().Width, envMap->GetDesc().Height);
        }

        // CDF Conditional Pass
        {
            GPU_SCOPE(cmdList, "EnvMap CDF Conditional");

            m_setCdfConditional.SetSRV_Tex2D(d3d->GetDevice(), 0, &m_envMapPmf, m_envMapPmf.GetDesc().Format);
            m_setCdfConditional.SetUAV_Tex2D(d3d->GetDevice(), 0, &m_envMapCdfConditional, m_envMapCdfConditional.GetDesc().Format);

            m_envMapCdfConditional.Transition(cmdList, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);

            heap->Bind(cmdList);
            cmdList->SetComputeRootSignature(m_rootSigSrvUav.Get());
            cmdList->SetPipelineState(m_pipelineCdfConditional.GetPSO());
            m_setCdfConditional.TransitionAllSRVToShaderResource(cmdList);
            m_setCdfConditional.SetDescriptorTables_Compute(cmdList);

            DispatchOverTexture(cmdList, 64, m_envMapPmf.GetDesc().Width);
        }

        // CDF Marginal Pass
        {
            GPU_SCOPE(cmdList, "EnvMap CDF Marginal");

            m_setCdfMarginal.SetSRV_Tex2D(d3d->GetDevice(), 0, &m_envMapCdfConditional, m_envMapCdfConditional.GetDesc().Format);
            m_setCdfMarginal.SetUAV_Tex1D(d3d->GetDevice(), 0, &m_envMapCdfMarginal, m_envMapCdfMarginal.GetDesc().Format);

            m_envMapCdfMarginal.Transition(cmdList, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);

            heap->Bind(cmdList);
            cmdList->SetComputeRootSignature(m_rootSigSrvUav.Get());
            cmdList->SetPipelineState(m_pipelineCdfMarginal.GetPSO());
            m_setCdfMarginal.TransitionAllSRVToShaderResource(cmdList);
            m_setCdfMarginal.SetDescriptorTables_Compute(cmdList);

            cmdList->Dispatch(1, 1, 1);

            const auto uavBarrier = CD3DX12_RESOURCE_BARRIER::UAV(m_envMapCdfMarginal.GetResource());
            cmdList->ResourceBarrier(1, &uavBarrier);
        }

        // CDF Marginal Normalize Pass
        {
            GPU_SCOPE(cmdList, "EnvMap CDF Marginal Normalize");

            m_setCdfMarginalNormalize.SetUAV_Tex1D(d3d->GetDevice(), 0, &m_envMapCdfMarginal, m_envMapCdfMarginal.GetDesc().Format);

            m_envMapCdfMarginal.Transition(cmdList, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);

            heap->Bind(cmdList);
            cmdList->SetComputeRootSignature(m_rootSigCdfMarginalNormalize.Get());
            cmdList->SetPipelineState(m_pipelineCdfMarginalNormalize.GetPSO());
            m_setCdfMarginalNormalize.TransitionAllSRVToShaderResource(cmdList);
            m_setCdfMarginalNormalize.SetDescriptorTables_Compute(cmdList);

            DispatchOverTexture(cmdList, 64, m_envMapCdfMarginal.GetDesc().Width);

            const auto uavBarrier = CD3DX12_RESOURCE_BARRIER::UAV(m_envMapCdfMarginal.GetResource());
            cmdList->ResourceBarrier(1, &uavBarrier);
        }

        m_envMapPmf.Transition(cmdList, D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE);
        m_envMapCdfConditional.Transition(cmdList, D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE);
        m_envMapCdfMarginal.Transition(cmdList, D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE);

        V(cmdList->Close());
        d3d->ExecuteCommandList(cmdList);
        d3d->Flush();
    }
}

void LightImportanceSampler::initializeResources(const D3D* d3d, Heap* heap, const D12Resource* envMap)
{
    // Root Sigs
    m_rootSigSrvUav.                     SmartInit(d3d->GetDevice(), 0, 1, 1);
    m_rootSigPmf.                        SmartInit(d3d->GetDevice(), 1, 1, 1);
    m_rootSigCdfMarginalNormalize.       SmartInit(d3d->GetDevice(), 0, 0, 1);

    // Sets
    m_setSumLum.                         Init(heap);
    m_setPmf.                            Init(heap);
    m_setCdfConditional.                 Init(heap);
    m_setCdfMarginal.                    Init(heap);
    m_setCdfMarginalNormalize.           Init(heap);

    // Pipelines
    m_pipelineSumLum.                    InitCompute(d3d->GetDevice(), "Compute/MIS/SumLumReductionSearchCS.hlsl", m_rootSigSrvUav.Get());
    m_pipelinePmf.                       InitCompute(d3d->GetDevice(), "Compute/MIS/EnvMapPmfCS.hlsl", m_rootSigPmf.Get());
    m_pipelineCdfConditional.            InitCompute(d3d->GetDevice(), "Compute/MIS/EnvMapCdfConditionalCS.hlsl", m_rootSigSrvUav.Get());
    m_pipelineCdfMarginal.               InitCompute(d3d->GetDevice(), "Compute/MIS/EnvMapCdfMarginalCS.hlsl", m_rootSigSrvUav.Get());
    m_pipelineCdfMarginalNormalize.      InitCompute(d3d->GetDevice(), "Compute/MIS/EnvMapCdfMarginalNormalizeCS.hlsl", m_rootSigCdfMarginalNormalize.Get());

    const size_t w = envMap->GetDesc().Width;
    const size_t h = envMap->GetDesc().Height;

    constexpr uint32_t blockCoverage = 32 * 9;
    m_sumLumGroupsX = (w + blockCoverage - 1) / blockCoverage;
    m_sumLumGroupsY = (h + blockCoverage - 1) / blockCoverage;
    m_sumLumBufferNumElements = m_sumLumGroupsX * m_sumLumGroupsY;
    m_sumLumBufferSize = m_sumLumBufferNumElements * sizeof(float);

    // D12Resources
    m_envMapSumLumBufferRW.         Init_Buffer("Env Map Sum Luminance Buffer (RW)", d3d->GetDevice(), m_sumLumBufferSize, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);
    m_envMapSumLumBufferReadback.   Init_Buffer("Env Map Sum Luminance Buffer (Readback)", d3d->GetDevice(), m_sumLumBufferSize, D3D12_RESOURCE_FLAG_NONE, true);
    m_envMapPmf.                    Init_Tex2D("Env Map PMF", d3d->GetDevice(), w, h, 1, DXGI_FORMAT_R32_FLOAT, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);
    m_envMapCdfConditional.         Init_Tex2D("Env Map CDF Conditional", d3d->GetDevice(), w, h, 1, DXGI_FORMAT_R32_FLOAT, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);
    m_envMapCdfMarginal.            Init_Tex1D("Env Map CDF Marginal", d3d->GetDevice(), w, 1, 1, DXGI_FORMAT_R32_FLOAT, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);
}
