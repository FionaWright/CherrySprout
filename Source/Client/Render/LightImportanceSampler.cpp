#include "System/pch.h"

#include "Render/LightImportanceSampler.h"

#include "Debug/GPUEventScoped.h"
#include "HWI/D3D.h"
#include "HWI/Heap.h"
#include "HWI/UploadHeap.h"
#include "PathTracing/NEE/LSD/Alias.h"
#include "Render/EnvironmentMap.h"
#include "Scene/InstanceData.h"
#include "Scene/Scene.h"
#include "Utils/CBVs.h"
#include "Utils/D3DUtils.h"
#include "Utils/Helper.h"

#define PUNCTUALS_ENABLED 1
#define ENV_ENABLED 1
#define EMISSIVE_ENABLED 1

void LightImportanceSampler::Build(D3D* d3d, Heap* heap, D12Resource* envMap, Scene* scene, const LsdConfig& config)
{
    if (!m_isInitialized)
    {
        initializeResources(d3d, envMap);
    }

    if (!m_sceneDataLoaded)
    {
        loadSceneData(d3d, heap, scene, envMap, config.UseAliasTables);
    }

    if (config.PunctualScale > 0.0f && PUNCTUALS_ENABLED)
        buildPunctualDistributions(d3d, heap, scene);
    m_punctualWeight *= max(0.0f, config.PunctualScale);

    if (config.EnvMapScale > 0.0f && ENV_ENABLED)
        buildEnvMapDistributions(d3d, heap, envMap);
    m_envMapWeight *= max(0.0f, config.EnvMapScale);
    m_envMapWeight /= 10000.0f;

    if (config.EmissiveScale > 0.0f && EMISSIVE_ENABLED)
        buildEmissiveDistributions(d3d, heap, scene);
    m_emissiveWeight *= max(0.0f, config.EmissiveScale);

    m_lsdIsAliasTable = config.UseAliasTables;

    // Combined Lights CDF Pass
    {
        d3d->Flush();
        const auto cmdListPtr = d3d->GetAvailableCmdList(D3D12_COMMAND_LIST_TYPE_DIRECT);
        const auto cmdList = cmdListPtr.Get();
        {
            GPU_SCOPE(cmdList, "Combined Lights CDF");

            CbvTotalLuminances cbv{};
            cbv.EnvMapTotalLuminance = m_envMapWeight;
            cbv.PunctualTotalLuminance = m_punctualWeight;
            cbv.EmissiveTotalLuminance = m_emissiveWeight;
            m_setLSD.UpdateCBV(0, &cbv);

            const size_t lsdStride = config.UseAliasTables ? sizeof(AliasEntry) : sizeof(ProbabilityDistributionSample);
            const Pipeline& pipeline = config.UseAliasTables ? m_pipelineLsdAlias : m_pipelineLsdCdf;

            m_setLSD.SetUAV_Buffer(d3d->GetDevice(), 0, &m_punctualPdfRW, m_maxPunctuals, sizeof(float));
            m_setLSD.SetUAV_Buffer(d3d->GetDevice(), 1, &m_emissivePdfRW, m_maxEmissives, sizeof(float));
            m_setLSD.SetUAV_Buffer(d3d->GetDevice(), 2, &m_lsdRW, m_maxLsdCount, lsdStride);

            m_punctualPdfRW.Transition(cmdList, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
            m_lsdRW.Transition(cmdList, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);

            heap->Bind(cmdList);
            cmdList->SetComputeRootSignature(m_rootSigCbvUav3.Get());
            cmdList->SetPipelineState(pipeline.GetPSO());
            m_setLSD.SetDescriptorTables_Compute(cmdList);

            cmdList->Dispatch(1, 1, 1);

            m_lsdRW.UavBarrier(cmdList);

#if CHERRY_DEBUG_FEATURES_ENABLED
            m_lsdRW.Transition(cmdList, D3D12_RESOURCE_STATE_COPY_SOURCE);
            m_lsdReadback.Transition(cmdList, D3D12_RESOURCE_STATE_COPY_DEST);
            cmdList->CopyBufferRegion(m_lsdReadback.GetResource(), 0, m_lsdRW.GetResource(), 0, m_lsdBufferSize);
#endif
        }
        V(cmdList->Close());
        d3d->ExecuteCommandList(cmdList);
        d3d->Flush();
    }


#if CHERRY_DEBUG_FEATURES_ENABLED
    if (m_maxLsdCount == 0)
    {
        m_cpuLsdAlias.clear();
        m_cpuLsdCdf.clear();
    }
    else if (config.UseAliasTables)
    {
        m_cpuLsdAlias.clear();
        m_cpuLsdAlias.resize(m_maxLsdCount);
        m_lsdReadback.Readback(m_cpuLsdAlias.data());
    }
    else
    {
        m_cpuLsdCdf.clear();
        m_cpuLsdCdf.resize(m_maxLsdCount);
        m_lsdReadback.Readback(m_cpuLsdCdf.data());
    }
#endif
}

uint32_t LightImportanceSampler::GetLsdBasePunctuals() const
{
    if (m_envMapWeight <= 0.0f)
        return 0;
    return 1;
}

uint32_t LightImportanceSampler::GetLsdBaseEmissives() const
{
    const uint32_t numPunctuals = m_punctualWeight <= 0.0f ? 0 : m_numPunctuals;
    return GetLsdBasePunctuals() + numPunctuals;
}

uint32_t LightImportanceSampler::GetLsdCount() const
{
    const uint32_t numEmissives = m_emissiveWeight <= 0.0f ? 0 : m_numEmissiveInstances;
    return GetLsdBaseEmissives() + numEmissives;
}

void LightImportanceSampler::buildPunctualDistributions(D3D* d3d, const Heap* heap, Scene* scene)
{
    m_numPunctuals = scene->CPU.MegaBufferPunctualLightsCount;

    // Punctual PDF Pass
    {
        const auto cmdListPtr = d3d->GetAvailableCmdList(D3D12_COMMAND_LIST_TYPE_DIRECT);
        const auto cmdList = cmdListPtr.Get();
        {
            GPU_SCOPE(cmdList, "Punctual Light PDF");

            D12Resource* punctualLights = &scene->GPU.MegaBufferPunctualLights;

            punctualLights->Transition(cmdList, D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE);
            m_punctualPdfRW.Transition(cmdList, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);

            heap->Bind(cmdList);
            cmdList->SetComputeRootSignature(m_rootSigSrvUav.Get());
            cmdList->SetPipelineState(m_pipelinePunctualPdf.GetPSO());
            m_setPunctualPdf.SetDescriptorTables_Compute(cmdList);

            DispatchOverTexture(cmdList, 64, m_numPunctuals);

            m_punctualPdfRW.Transition(cmdList, D3D12_RESOURCE_STATE_COPY_SOURCE);
            m_punctualPdfReadback.Transition(cmdList, D3D12_RESOURCE_STATE_COPY_DEST);
            cmdList->CopyBufferRegion(m_punctualPdfReadback.GetResource(), 0, m_punctualPdfRW.GetResource(), 0, m_punctualLightsPdfBufferSize);
        }
        V(cmdList->Close());
        d3d->ExecuteCommandList(cmdList);
        d3d->Flush();
    }

    std::vector<float> buff(m_maxPunctuals);
    m_punctualPdfReadback.Readback(buff.data());

    m_punctualWeight = 0.0f;
    for (int i = 0; i < m_numPunctuals; i++)
    {
        const float lum = buff[i];
        CherryAssert(!std::isnan(lum));
        m_punctualWeight += lum;
    }
}

void LightImportanceSampler::buildEmissiveDistributions(D3D* d3d, const Heap* heap, Scene* scene)
{
    const size_t emissiveMapBytes = m_maxEmissives * sizeof(uint32_t);
    const size_t emissivePdfBytes = m_maxEmissives * sizeof(float);

    if (!m_emissiveInstanceMap.IsInitialized() || emissiveMapBytes != m_emissiveInstanceMap.GetDesc().Width)
    {
        m_emissiveInstanceMap.Reset();
        m_emissivePdfRW.Reset();
        m_emissivePdfReadback.Reset();

        m_emissiveInstanceMap.Init_Buffer("Emissive Instance Map", d3d->GetDevice(), emissiveMapBytes);
        m_emissivePdfRW.Init_Buffer("Emissive PDF (RW)", d3d->GetDevice(), emissivePdfBytes, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);
        m_emissivePdfReadback.Init_Buffer("Emissive PDF (Readback)", d3d->GetDevice(), emissivePdfBytes, D3D12_RESOURCE_FLAG_NONE, true);
    }

    std::vector<uint32_t> emissiveInstanceMap;

    for (int instanceIdx = 0; instanceIdx < scene->CPU.ObjectCount; instanceIdx++)
    {
        const Object& obj = scene->CPU.Objects[instanceIdx];
        const uint32_t materialIdx = obj.MaterialIndex;
        const Material& mat = scene->CPU.MegaBufferMaterials[materialIdx];

        if (!IsMaterialEmissive(mat))
            continue;

        emissiveInstanceMap.emplace_back(instanceIdx);
    }

    if (emissiveInstanceMap.empty())
    {
        m_numEmissiveInstances = 0;
        m_emissiveWeight = 0.0f;
        return;
    }

    m_numEmissiveInstances = emissiveInstanceMap.size();

    UploadHeap uploadHeap;
    uploadHeap.Init(d3d->GetDevice(), Align(emissiveMapBytes, 512));

    d3d->Flush();
    const auto cmdListPtr = d3d->GetAvailableCmdList(D3D12_COMMAND_LIST_TYPE_DIRECT);
    const auto cmdList = cmdListPtr.Get();
    {
        GPU_SCOPE(cmdList, "PDF: Emissive");

        m_emissiveInstanceMap.UploadBuffer(cmdList, &uploadHeap, emissiveInstanceMap.data(), emissiveMapBytes);

        m_emissiveInstanceMap.Transition(cmdList, D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE);
        m_emissivePdfRW.Transition(cmdList, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);

        m_setEmissivePdf.SetSRV_Buffer(d3d->GetDevice(), 0, &scene->GPU.MegaBufferInstanceData, scene->CPU.ObjectCount, sizeof(InstanceData));
        m_setEmissivePdf.SetSRV_Buffer(d3d->GetDevice(), 1, &scene->GPU.MegaBufferMaterials, scene->CPU.MegaBufferMaterialsCount, sizeof(Material));
        m_setEmissivePdf.SetSRV_Buffer(d3d->GetDevice(), 2, &scene->GPU.MegaBufferVertex, scene->CPU.MegaBufferVertexCount, sizeof(Vertex));
        m_setEmissivePdf.SetSRV_Buffer(d3d->GetDevice(), 3, &scene->GPU.MegaBufferIndex, scene->CPU.MegaBufferIndexCount, sizeof(uint32_t));
        m_setEmissivePdf.SetSRV_Buffer(d3d->GetDevice(), 4, &m_emissiveInstanceMap, m_numEmissiveInstances, sizeof(uint32_t));
        m_setEmissivePdf.SetUAV_Buffer(d3d->GetDevice(), 0, &m_emissivePdfRW, m_numEmissiveInstances, sizeof(float));

        CbvEmissivePdf cbv;
        cbv.NumEmissiveInstances = m_numEmissiveInstances;
        m_setEmissivePdf.UpdateCBV(0, &cbv);

        heap->Bind(cmdList);
        cmdList->SetComputeRootSignature(m_rootSigCbvSrv5Uav.Get());
        cmdList->SetPipelineState(m_pipelineEmissivePdf.GetPSO());
        m_setEmissivePdf.SetDescriptorTables_Compute(cmdList);

        DispatchOverTexture(cmdList, 32, m_numEmissiveInstances);

        m_emissivePdfReadback.Transition(cmdList, D3D12_RESOURCE_STATE_COPY_DEST);
        m_emissivePdfRW.Transition(cmdList, D3D12_RESOURCE_STATE_COPY_SOURCE);

        cmdList->CopyBufferRegion(m_emissivePdfReadback.GetResource(), 0, m_emissivePdfRW.GetResource(), 0, emissivePdfBytes);
    }
    V(cmdList->Close());
    d3d->ExecuteCommandList(cmdList);
    d3d->Flush();

    std::vector<float> buff(m_maxEmissives);
    m_emissivePdfReadback.Readback(buff.data());

    m_emissiveWeight = 0.0f;
    for (int i = 0; i < m_numEmissiveInstances; i++)
    {
        const float lum = buff[i];
        CherryAssert(!std::isnan(lum));
        m_emissiveWeight += lum;
    }

    if (m_emissiveWeight <= 0.0f)
        m_numEmissiveInstances = 0;
}

void LightImportanceSampler::buildEnvMapDistributions(D3D* d3d, const Heap* heap, const D12Resource* envMap)
{
    // Get Sum of Luminance Pass
    {
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

        m_envMapWeight = 0;
        for (int i = 0; i < m_sumLumBufferNumElements; i++)
        {
            m_envMapWeight += buff[i];
        }
    }

    // PDF & CDF Passes
    {
        const auto cmdListPtr = d3d->GetAvailableCmdList(D3D12_COMMAND_LIST_TYPE_DIRECT);
        const auto cmdList = cmdListPtr.Get();

        // PDF Pass
        {
            GPU_SCOPE(cmdList, "EnvMap PDF");

            m_setEnvMapPdf.UpdateCBV(0, &m_envMapWeight);
            m_envMapPdf.Transition(cmdList, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);

            heap->Bind(cmdList);
            cmdList->SetComputeRootSignature(m_rootSigCbvSrvUav.Get());
            cmdList->SetPipelineState(m_pipelineEnvMapPdf.GetPSO());
            m_setEnvMapPdf.TransitionAllSRVToShaderResource(cmdList);
            m_setEnvMapPdf.SetDescriptorTables_Compute(cmdList);

            DispatchOverTexture(cmdList, 16, envMap->GetDesc().Width, envMap->GetDesc().Height);
        }

        // CDF Conditional Pass
        {
            GPU_SCOPE(cmdList, "EnvMap CDF Conditional");

            m_envMapCdfConditional.Transition(cmdList, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);

            heap->Bind(cmdList);
            cmdList->SetComputeRootSignature(m_rootSigSrvUav.Get());
            cmdList->SetPipelineState(m_pipelineEnvMapCdfConditional.GetPSO());
            m_setEnvMapCdfConditional.TransitionAllSRVToShaderResource(cmdList);
            m_setEnvMapCdfConditional.SetDescriptorTables_Compute(cmdList);

            DispatchOverTexture(cmdList, 64, m_envMapPdf.GetDesc().Width);

            m_envMapCdfConditional.UavBarrier(cmdList);
        }

        // CDF Marginal Pass
        {
            GPU_SCOPE(cmdList, "EnvMap CDF Marginal");

            m_envMapCdfMarginal.Transition(cmdList, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);

            heap->Bind(cmdList);
            cmdList->SetComputeRootSignature(m_rootSigSrvUav.Get());
            cmdList->SetPipelineState(m_pipelineEnvMapCdfMarginal.GetPSO());
            m_setEnvMapCdfMarginal.TransitionAllSRVToShaderResource(cmdList);
            m_setEnvMapCdfMarginal.SetDescriptorTables_Compute(cmdList);

            cmdList->Dispatch(1, 1, 1);

            m_envMapCdfMarginal.UavBarrier(cmdList);
        }

        // CDF Conditional Normalize Pass
        {
            GPU_SCOPE(cmdList, "EnvMap CDF Conditional Normalize");

            m_envMapCdfConditional.Transition(cmdList, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);

            heap->Bind(cmdList);
            cmdList->SetComputeRootSignature(m_rootSigUav.Get());
            cmdList->SetPipelineState(m_pipelineEnvMapCdfConditionalNormalize.GetPSO());
            m_setEnvMapCdfConditionalNormalize.SetDescriptorTables_Compute(cmdList);

            DispatchOverTexture(cmdList, 64, m_envMapCdfConditional.GetDesc().Width);

            m_envMapCdfConditional.UavBarrier(cmdList);
        }

        // CDF Marginal Normalize Pass
        {
            GPU_SCOPE(cmdList, "EnvMap CDF Marginal Normalize");

            m_envMapCdfMarginal.Transition(cmdList, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);

            heap->Bind(cmdList);
            cmdList->SetComputeRootSignature(m_rootSigUav.Get());
            cmdList->SetPipelineState(m_pipelineCdfNormalize1D.GetPSO());
            m_setCdfNormalize1D.SetDescriptorTables_Compute(cmdList);

            DispatchOverTexture(cmdList, 64, m_envMapCdfMarginal.GetDesc().Width);

            m_envMapCdfMarginal.UavBarrier(cmdList);
        }

        m_envMapPdf.Transition(cmdList, D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE);
        m_envMapCdfConditional.Transition(cmdList, D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE);
        m_envMapCdfMarginal.Transition(cmdList, D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE);

        V(cmdList->Close());
        d3d->ExecuteCommandList(cmdList);
        d3d->Flush();
    }
}

void LightImportanceSampler::loadSceneData(D3D* d3d, Heap* heap, Scene* scene, D12Resource* envMap, const bool aliasTablesEnabled)
{
    d3d->Flush();

    m_uploadHeap.FreeAssignedData();

    m_maxPunctuals = scene->CPU.MegaBufferPunctualLightsCount;
    m_maxEmissives = scene->CPU.ObjectCount;
    constexpr size_t maxEnvMapCount = 1;
    m_maxLsdCount = m_maxEmissives + m_maxPunctuals + maxEnvMapCount;

    m_punctualLightsPdfBufferSize = m_maxPunctuals * sizeof(float);

    CherryAssert(m_maxLsdCount < ALIAS_TABLE_BUILD_MAX_STACK_SIZE);

    const size_t lsdStride = aliasTablesEnabled ? sizeof(AliasEntry) : sizeof(ProbabilityDistributionSample);
    m_lsdBufferSize = m_maxLsdCount * lsdStride;

    m_punctualPdfRW.Release();
    m_punctualPdfReadback.Release();
    m_lsdRW.Release();
    m_lsdReadback.Release();

    m_punctualPdfRW.          Init_Buffer("Punctual Light PDF (RW)", d3d->GetDevice(), m_punctualLightsPdfBufferSize, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);
    m_punctualPdfReadback.    Init_Buffer("Punctual Light PDF (Readback)", d3d->GetDevice(), m_punctualLightsPdfBufferSize, D3D12_RESOURCE_FLAG_NONE, true);
    m_lsdRW.                  Init_Buffer("LSD (RW)", d3d->GetDevice(), m_lsdBufferSize, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);
    m_lsdReadback.            Init_Buffer("LSD (Readback)", d3d->GetDevice(), m_lsdBufferSize, D3D12_RESOURCE_FLAG_NONE, true);

    // Sets
    m_setEnvMapSumLum.                          Init(heap);
    m_setEnvMapPdf.                             Init(heap);
    m_setEnvMapCdfConditional.                  Init(heap);
    m_setEnvMapCdfMarginal.                     Init(heap);
    m_setEnvMapCdfConditionalNormalize.         Init(heap);
    m_setCdfNormalize1D.                        Init(heap);
    m_setPunctualPdf.                           Init(heap);
    m_setEmissivePdf.                           Init(heap);
    m_setLSD.                                   Init(heap);

    m_setPunctualPdf.SetSRV_Buffer(d3d->GetDevice(), 0, &scene->GPU.MegaBufferPunctualLights, scene->CPU.MegaBufferPunctualLightsCount, sizeof(PunctualLight));
    m_setPunctualPdf.SetUAV_Buffer(d3d->GetDevice(), 0, &m_punctualPdfRW, scene->CPU.MegaBufferPunctualLightsCount, sizeof(float));

    m_setEmissivePdf.AddCBV(d3d->GetDevice(), sizeof(CbvEmissivePdf), &m_uploadHeap);
    m_setLSD.AddCBV(d3d->GetDevice(), sizeof(CbvTotalLuminances), &m_uploadHeap);

    m_setEnvMapSumLum.SetSRV_Tex2D(d3d->GetDevice(), 0, envMap, envMap->GetDesc().Format);
    m_setEnvMapSumLum.SetUAV_Buffer(d3d->GetDevice(), 0, &m_envMapSumLumBufferRW, m_sumLumBufferNumElements, sizeof(float));

    m_setEnvMapPdf.AddCBV(d3d->GetDevice(), sizeof(float), &m_uploadHeap);
    m_setEnvMapPdf.SetSRV_Tex2D(d3d->GetDevice(), 0, envMap, envMap->GetDesc().Format);
    m_setEnvMapPdf.SetUAV_Tex2D(d3d->GetDevice(), 0, &m_envMapPdf, m_envMapPdf.GetDesc().Format);

    m_setEnvMapCdfConditional.SetSRV_Tex2D(d3d->GetDevice(), 0, &m_envMapPdf, m_envMapPdf.GetDesc().Format);
    m_setEnvMapCdfConditional.SetUAV_Tex2D(d3d->GetDevice(), 0, &m_envMapCdfConditional, m_envMapCdfConditional.GetDesc().Format);

    m_setEnvMapCdfMarginal.SetSRV_Tex2D(d3d->GetDevice(), 0, &m_envMapCdfConditional, m_envMapCdfConditional.GetDesc().Format);
    m_setEnvMapCdfMarginal.SetUAV_Tex1D(d3d->GetDevice(), 0, &m_envMapCdfMarginal, m_envMapCdfMarginal.GetDesc().Format);

    m_setEnvMapCdfConditionalNormalize.SetUAV_Tex2D(d3d->GetDevice(), 0, &m_envMapCdfConditional, m_envMapCdfConditional.GetDesc().Format);

    m_setCdfNormalize1D.SetUAV_Tex1D(d3d->GetDevice(), 0, &m_envMapCdfMarginal, m_envMapCdfMarginal.GetDesc().Format);

    m_sceneDataLoaded = true;
}

void LightImportanceSampler::initializeResources(const D3D* d3d, const D12Resource* envMap)
{
    // Root Sigs
    m_rootSigUav.                               SmartInit(d3d->GetDevice(), 0, 0, 1);
    m_rootSigSrvUav.                            SmartInit(d3d->GetDevice(), 0, 1, 1);
    m_rootSigCbvSrv5Uav.                        SmartInit(d3d->GetDevice(), 1, 5, 1);
    m_rootSigCbvSrvUav.                         SmartInit(d3d->GetDevice(), 1, 1, 1);
    m_rootSigCbvUav3.                           SmartInit(d3d->GetDevice(), 1, 0, 3);

    // Pipelines
    m_pipelineEnvMapSumLum.                     InitCompute(d3d->GetDevice(), "Compute/NEE/EnvironmentMap/SumLumReductionSearchCS.hlsl", m_rootSigSrvUav.Get());
    m_pipelineEnvMapPdf.                        InitCompute(d3d->GetDevice(), "Compute/NEE/EnvironmentMap/PdfCS.hlsl", m_rootSigCbvSrvUav.Get());
    m_pipelineEnvMapCdfConditional.             InitCompute(d3d->GetDevice(), "Compute/NEE/EnvironmentMap/CdfConditionalCS.hlsl", m_rootSigSrvUav.Get());
    m_pipelineEnvMapCdfMarginal.                InitCompute(d3d->GetDevice(), "Compute/NEE/EnvironmentMap/CdfMarginalCS.hlsl", m_rootSigSrvUav.Get());
    m_pipelineEnvMapCdfConditionalNormalize.    InitCompute(d3d->GetDevice(), "Compute/NEE/EnvironmentMap/CdfConditionalNormalizeCS.hlsl", m_rootSigUav.Get());

    m_pipelineCdfNormalize1D.                   InitCompute(d3d->GetDevice(), "Compute/NEE/CdfNormalize1DCS.hlsl", m_rootSigUav.Get());
    m_pipelinePunctualPdf.                      InitCompute(d3d->GetDevice(), "Compute/NEE/PunctualPdfCS.hlsl", m_rootSigSrvUav.Get());

    m_pipelineEmissivePdf.                      InitCompute(d3d->GetDevice(), "Compute/NEE/EmissivePdfCS.hlsl", m_rootSigCbvSrv5Uav.Get());

    m_pipelineLsdCdf.                           InitCompute(d3d->GetDevice(), "Compute/NEE/LightCdfCS.hlsl", m_rootSigCbvUav3.Get());
    m_pipelineLsdAlias.                         InitCompute(d3d->GetDevice(), "Compute/NEE/LightAliasCS.hlsl", m_rootSigCbvUav3.Get(), {ALIAS_TABLE_BUILD_MAX_STACK_SIZE_DEFINE});

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
    m_envMapPdf.                                Init_Tex2D("Env Map PDF", d3d->GetDevice(), w, h, 1, distributionFormat, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);
    m_envMapCdfConditional.                     Init_Tex2D("Env Map CDF Conditional", d3d->GetDevice(), w, h, 1, distributionFormat, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);
    m_envMapCdfMarginal.                        Init_Tex1D("Env Map CDF Marginal", d3d->GetDevice(), w, 1, 1, distributionFormat, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);

    size_t uploadHeapSize = 0;
    uploadHeapSize += Align(sizeof(CbvTotalLuminances), 256);
    uploadHeapSize += Align(sizeof(CbvEmissivePdf), 256);
    uploadHeapSize += Align(sizeof(float), 256);
    m_uploadHeap.Init(d3d->GetDevice(), uploadHeapSize);

    m_isInitialized = true;
}
