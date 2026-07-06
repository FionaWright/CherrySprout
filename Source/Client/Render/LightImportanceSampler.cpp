#include "System/pch.h"

#include "Render/LightImportanceSampler.h"

#include "Debug/GPUEventScoped.h"
#include "HWI/D3D.h"
#include "HWI/Heap.h"
#include "HWI/UploadHeap.h"
#include "Scene/Scene.h"
#include "Utils/CBVs.h"
#include "Utils/D3DUtils.h"
#include "Utils/Helper.h"

void LightImportanceSampler::Build(D3D* d3d, Heap* heap, D12Resource* envMap, Scene* scene)
{
    if (!m_envMapPmf.IsInitialized())
    {
        initializeResources(d3d, heap, envMap, scene);
    }

    buildEnvMapDistributions(d3d, heap, envMap);

    return;

    // Punctual PMF Pass
    {
        const auto cmdListPtr = d3d->GetAvailableCmdList(D3D12_COMMAND_LIST_TYPE_DIRECT);
        const auto cmdList = cmdListPtr.Get();
        {
            GPU_SCOPE(cmdList, "Punctual Light PMF");

            D12Resource* punctualLights = &scene->GPU.MegaBufferPunctualLights;

            m_setPunctualPmf.SetSRV_Buffer(d3d->GetDevice(), 0, punctualLights, scene->CPU.MegaBufferPunctualLightsCount, sizeof(PunctualLight));
            m_setPunctualPmf.SetUAV_Buffer(d3d->GetDevice(), 0, &m_punctualPmfRW, scene->CPU.MegaBufferPunctualLightsCount, sizeof(float));

            punctualLights->Transition(cmdList, D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE);
            m_punctualPmfRW.Transition(cmdList, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);

            heap->Bind(cmdList);
            cmdList->SetComputeRootSignature(m_rootSigSrvUav.Get());
            cmdList->SetPipelineState(m_pipelinePunctualPmf.GetPSO());
            m_setPunctualPmf.SetDescriptorTables_Compute(cmdList);

            DispatchOverTexture(cmdList, 64, scene->CPU.MegaBufferPunctualLightsCount);

            m_punctualPmfRW.Transition(cmdList, D3D12_RESOURCE_STATE_COPY_SOURCE);
            m_punctualPmfReadback.Transition(cmdList, D3D12_RESOURCE_STATE_COPY_DEST);
            cmdList->CopyBufferRegion(m_punctualPmfReadback.GetResource(), 0, m_punctualPmfRW.GetResource(), 0, m_punctualLightsPmfBufferSize);
        }
        V(cmdList->Close());
        d3d->ExecuteCommandList(cmdList);
        d3d->Flush();
    }

    std::vector<float> buff(scene->CPU.MegaBufferPunctualLightsCount);
    m_punctualPmfReadback.Readback(buff.data());

    for (const float lum : buff)
    {
        CherryAssert(!std::isnan(lum));

        m_punctualTotalLuminance += lum;
    }

    UploadHeap uploadHeap;
    uploadHeap.Init(d3d->GetDevice(), Align(sizeof(CbvTotalLuminances), 256));

    const auto cmdListPtr = d3d->GetAvailableCmdList(D3D12_COMMAND_LIST_TYPE_DIRECT);
    const auto cmdList = cmdListPtr.Get();

    // Combined Lights CDF Pass
    {
        GPU_SCOPE(cmdList, "Combined Lights CDF");

        m_setLightCdf.SetUAV_Buffer(d3d->GetDevice(), 0, &m_punctualPmfRW, scene->CPU.MegaBufferPunctualLightsCount, sizeof(float));
        m_setLightCdf.SetUAV_Tex1D(d3d->GetDevice(), 1, &m_lightCdf, m_lightCdf.GetDesc().Format);

        CbvTotalLuminances cbv{};
        cbv.EnvMapTotalLuminance = m_envMapTotalLuminance;
        cbv.PunctualTotalLuminance = m_punctualTotalLuminance;
        m_setLightCdf.AddCBV(d3d->GetDevice(), sizeof(CbvTotalLuminances), &uploadHeap);
        m_setLightCdf.UpdateCBV(0, &cbv);

        m_punctualPmfRW.Transition(cmdList, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
        m_lightCdf.Transition(cmdList, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);

        heap->Bind(cmdList);
        cmdList->SetComputeRootSignature(m_rootSigCbvUav2.Get());
        cmdList->SetPipelineState(m_pipelineLightCdf.GetPSO());
        m_setLightCdf.SetDescriptorTables_Compute(cmdList);

        cmdList->Dispatch(1, 1, 1);

        const auto uavBarrier = CD3DX12_RESOURCE_BARRIER::UAV(m_lightCdf.GetResource());
        cmdList->ResourceBarrier(1, &uavBarrier);
    }

    // Combined Lights CDF Normalize Pass
    {
        GPU_SCOPE(cmdList, "Combined Lights CDF Normalize");

        m_setCdfNormalize1D.SetUAV_Tex1D(d3d->GetDevice(), 0, &m_lightCdf, m_lightCdf.GetDesc().Format);

        m_lightCdf.Transition(cmdList, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);

        heap->Bind(cmdList);
        cmdList->SetComputeRootSignature(m_rootSigUav.Get());
        cmdList->SetPipelineState(m_pipelineCdfNormalize1D.GetPSO());
        m_setCdfNormalize1D.SetDescriptorTables_Compute(cmdList);

        DispatchOverTexture(cmdList, 64, m_lightCdf.GetDesc().Width);
    }

    V(cmdList->Close());
    d3d->ExecuteCommandList(cmdList);
    d3d->Flush();
}

void LightImportanceSampler::buildEnvMapDistributions(D3D* d3d, Heap* heap, D12Resource* envMap)
{
    // Get Sum of Luminance Pass
    {
        m_setEnvMapSumLum.SetSRV_Tex2D(d3d->GetDevice(), 0, envMap, envMap->GetDesc().Format);
        m_setEnvMapSumLum.SetUAV_Buffer(d3d->GetDevice(), 0, &m_envMapSumLumBufferRW, m_sumLumBufferNumElements, sizeof(float));

        d3d->Flush();
        {
            const auto cmdListPtr = d3d->GetAvailableCmdList(D3D12_COMMAND_LIST_TYPE_DIRECT);
            const auto cmdList = cmdListPtr.Get();

            {
                GPU_SCOPE(cmdList, "CDF: EnvMap Sum of Luminance Reduction Search");

                m_envMapSumLumBufferRW.Transition(cmdList, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);

                heap->Bind(cmdList);
                cmdList->SetComputeRootSignature(m_rootSigSrvUav.Get());
                cmdList->SetPipelineState(m_pipelineEnvMapSumLum.GetPSO());
                m_setEnvMapSumLum.TransitionAllSRVToShaderResource(cmdList);
                m_setEnvMapSumLum.SetDescriptorTables_Compute(cmdList);

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

        m_envMapTotalLuminance = 0;
        for (int i = 0; i < m_sumLumBufferNumElements; i++)
        {
            m_envMapTotalLuminance += buff[i];
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

            m_setEnvMapPmf.AddCBV(d3d->GetDevice(), sizeof(float), &uploadHeap);
            m_setEnvMapPmf.SetSRV_Tex2D(d3d->GetDevice(), 0, envMap, envMap->GetDesc().Format);
            m_setEnvMapPmf.SetUAV_Tex2D(d3d->GetDevice(), 0, &m_envMapPmf, m_envMapPmf.GetDesc().Format);

            m_setEnvMapPmf.UpdateCBV(0, &m_envMapTotalLuminance);
            m_envMapPmf.Transition(cmdList, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);

            heap->Bind(cmdList);
            cmdList->SetComputeRootSignature(m_rootSigCbvSrvUav.Get());
            cmdList->SetPipelineState(m_pipelineEnvMapPmf.GetPSO());
            m_setEnvMapPmf.TransitionAllSRVToShaderResource(cmdList);
            m_setEnvMapPmf.SetDescriptorTables_Compute(cmdList);

            DispatchOverTexture(cmdList, 16, envMap->GetDesc().Width, envMap->GetDesc().Height);
        }

        // CDF Conditional Pass
        {
            GPU_SCOPE(cmdList, "EnvMap CDF Conditional");

            m_setEnvMapCdfConditional.SetSRV_Tex2D(d3d->GetDevice(), 0, &m_envMapPmf, m_envMapPmf.GetDesc().Format);
            m_setEnvMapCdfConditional.SetUAV_Tex2D(d3d->GetDevice(), 0, &m_envMapCdfConditional, m_envMapCdfConditional.GetDesc().Format);

            m_envMapCdfConditional.Transition(cmdList, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);

            heap->Bind(cmdList);
            cmdList->SetComputeRootSignature(m_rootSigSrvUav.Get());
            cmdList->SetPipelineState(m_pipelineEnvMapCdfConditional.GetPSO());
            m_setEnvMapCdfConditional.TransitionAllSRVToShaderResource(cmdList);
            m_setEnvMapCdfConditional.SetDescriptorTables_Compute(cmdList);

            DispatchOverTexture(cmdList, 64, m_envMapPmf.GetDesc().Width);

            const auto uavBarrier = CD3DX12_RESOURCE_BARRIER::UAV(m_envMapCdfConditional.GetResource());
            cmdList->ResourceBarrier(1, &uavBarrier);
        }

        // CDF Marginal Pass
        {
            GPU_SCOPE(cmdList, "EnvMap CDF Marginal");

            m_setEnvMapCdfMarginal.SetSRV_Tex2D(d3d->GetDevice(), 0, &m_envMapCdfConditional, m_envMapCdfConditional.GetDesc().Format);
            m_setEnvMapCdfMarginal.SetUAV_Tex1D(d3d->GetDevice(), 0, &m_envMapCdfMarginal, m_envMapCdfMarginal.GetDesc().Format);

            m_envMapCdfMarginal.Transition(cmdList, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);

            heap->Bind(cmdList);
            cmdList->SetComputeRootSignature(m_rootSigSrvUav.Get());
            cmdList->SetPipelineState(m_pipelineEnvMapCdfMarginal.GetPSO());
            m_setEnvMapCdfMarginal.TransitionAllSRVToShaderResource(cmdList);
            m_setEnvMapCdfMarginal.SetDescriptorTables_Compute(cmdList);

            cmdList->Dispatch(1, 1, 1);

            const auto uavBarrier = CD3DX12_RESOURCE_BARRIER::UAV(m_envMapCdfMarginal.GetResource());
            cmdList->ResourceBarrier(1, &uavBarrier);
        }

        // CDF Conditional Normalize Pass
        {
            GPU_SCOPE(cmdList, "EnvMap CDF Conditional Normalize");

            m_setEnvMapCdfConditionalNormalize.SetUAV_Tex2D(d3d->GetDevice(), 0, &m_envMapCdfConditional, m_envMapCdfConditional.GetDesc().Format);

            m_envMapCdfConditional.Transition(cmdList, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);

            heap->Bind(cmdList);
            cmdList->SetComputeRootSignature(m_rootSigUav.Get());
            cmdList->SetPipelineState(m_pipelineEnvMapCdfConditionalNormalize.GetPSO());
            m_setEnvMapCdfConditionalNormalize.SetDescriptorTables_Compute(cmdList);

            DispatchOverTexture(cmdList, 64, m_envMapCdfConditional.GetDesc().Width);

            const auto uavBarrier = CD3DX12_RESOURCE_BARRIER::UAV(m_envMapCdfConditional.GetResource());
            cmdList->ResourceBarrier(1, &uavBarrier);
        }

        // CDF Marginal Normalize Pass
        {
            GPU_SCOPE(cmdList, "EnvMap CDF Marginal Normalize");

            m_setCdfNormalize1D.SetUAV_Tex1D(d3d->GetDevice(), 0, &m_envMapCdfMarginal, m_envMapCdfMarginal.GetDesc().Format);

            m_envMapCdfMarginal.Transition(cmdList, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);

            heap->Bind(cmdList);
            cmdList->SetComputeRootSignature(m_rootSigUav.Get());
            cmdList->SetPipelineState(m_pipelineCdfNormalize1D.GetPSO());
            m_setCdfNormalize1D.SetDescriptorTables_Compute(cmdList);

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

void LightImportanceSampler::initializeResources(const D3D* d3d, Heap* heap, const D12Resource* envMap, const Scene* scene)
{
    // Root Sigs
    m_rootSigUav.                               SmartInit(d3d->GetDevice(), 0, 0, 1);
    m_rootSigSrvUav.                            SmartInit(d3d->GetDevice(), 0, 1, 1);
    m_rootSigCbvSrvUav.                         SmartInit(d3d->GetDevice(), 1, 1, 1);
    m_rootSigCbvUav2.                           SmartInit(d3d->GetDevice(), 1, 0, 2);

    // Sets
    m_setEnvMapSumLum.                          Init(heap);
    m_setEnvMapPmf.                             Init(heap);
    m_setEnvMapCdfConditional.                  Init(heap);
    m_setEnvMapCdfMarginal.                     Init(heap);
    m_setEnvMapCdfConditionalNormalize.         Init(heap);

    m_setCdfNormalize1D.                        Init(heap);
    m_setPunctualPmf.                           Init(heap);
    m_setLightCdf.                              Init(heap);

    // Pipelines
    m_pipelineEnvMapSumLum.                     InitCompute(d3d->GetDevice(), "Compute/NEE/EnvironmentMap/SumLumReductionSearchCS.hlsl", m_rootSigSrvUav.Get());
    m_pipelineEnvMapPmf.                        InitCompute(d3d->GetDevice(), "Compute/NEE/EnvironmentMap/PmfCS.hlsl", m_rootSigCbvSrvUav.Get());
    m_pipelineEnvMapCdfConditional.             InitCompute(d3d->GetDevice(), "Compute/NEE/EnvironmentMap/CdfConditionalCS.hlsl", m_rootSigSrvUav.Get());
    m_pipelineEnvMapCdfMarginal.                InitCompute(d3d->GetDevice(), "Compute/NEE/EnvironmentMap/CdfMarginalCS.hlsl", m_rootSigSrvUav.Get());
    m_pipelineEnvMapCdfConditionalNormalize.    InitCompute(d3d->GetDevice(), "Compute/NEE/EnvironmentMap/CdfConditionalNormalizeCS.hlsl", m_rootSigUav.Get());

    m_pipelineCdfNormalize1D.                   InitCompute(d3d->GetDevice(), "Compute/NEE/CdfNormalize1DCS.hlsl", m_rootSigUav.Get());
    m_pipelinePunctualPmf.                      InitCompute(d3d->GetDevice(), "Compute/NEE/PunctualPmfCS.hlsl", m_rootSigSrvUav.Get());
    m_pipelineLightCdf.                         InitCompute(d3d->GetDevice(), "Compute/NEE/LightCdfCS.hlsl", m_rootSigCbvUav2.Get());

    const size_t w = envMap->GetDesc().Width;
    const size_t h = envMap->GetDesc().Height;

    constexpr uint32_t blockCoverage = 32 * 9;
    m_sumLumGroupsX = (w + blockCoverage - 1) / blockCoverage;
    m_sumLumGroupsY = (h + blockCoverage - 1) / blockCoverage;
    m_sumLumBufferNumElements = m_sumLumGroupsX * m_sumLumGroupsY;
    m_sumLumBufferSize = m_sumLumBufferNumElements * sizeof(float);

    constexpr DXGI_FORMAT distributionFormat = DXGI_FORMAT_R32_FLOAT; // TODO: R16 might be enough?

    // D12Resources
    m_envMapSumLumBufferRW.                     Init_Buffer("Env Map Sum Luminance Buffer (RW)", d3d->GetDevice(), m_sumLumBufferSize, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);
    m_envMapSumLumBufferReadback.               Init_Buffer("Env Map Sum Luminance Buffer (Readback)", d3d->GetDevice(), m_sumLumBufferSize, D3D12_RESOURCE_FLAG_NONE, true);
    m_envMapPmf.                                Init_Tex2D("Env Map PMF", d3d->GetDevice(), w, h, 1, distributionFormat, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);
    m_envMapCdfConditional.                     Init_Tex2D("Env Map CDF Conditional", d3d->GetDevice(), w, h, 1, distributionFormat, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);
    m_envMapCdfMarginal.                        Init_Tex1D("Env Map CDF Marginal", d3d->GetDevice(), w, 1, 1, distributionFormat, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);

    const size_t punctualLightsCount = scene->CPU.MegaBufferPunctualLightsCount;
    m_punctualLightsPmfBufferSize = punctualLightsCount * sizeof(float);
    const size_t totalLightsCount = scene->CPU.MegaBufferPunctualLightsCount + 1;
    m_lightsCdfBufferSize = totalLightsCount * sizeof(float);

    m_punctualPmfRW.                            Init_Buffer("Punctual Light PMF (RW)", d3d->GetDevice(), m_punctualLightsPmfBufferSize, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);
    m_punctualPmfReadback.                      Init_Buffer("Punctual Light PMF (Readback)", d3d->GetDevice(), m_punctualLightsPmfBufferSize, D3D12_RESOURCE_FLAG_NONE, true);
    m_lightCdf.                                 Init_Tex1D("Combined Lights CDF", d3d->GetDevice(), totalLightsCount, 1, 1, distributionFormat, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);
}
