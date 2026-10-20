#include "System/pch.h"

#include "Render/RestirManager.h"

#include "Debug/GPUEventScoped.h"
#include "HWI/D3D.h"
#include "PathTracing/ReSTIR/Structs.h"
#include "System/Config.h"
#include "Utils/D3DUtils.h"

#define NUM_RESERVOIR_SUB_BUFFERS 3 // Ping, Pong, Prev
#define NUM_CONTEXT_SUB_BUFFERS 2 // Curr, Prev

void RestirManager::initResources(ID3D12Device* device)
{
    m_numReservoirs = Config::GetSystem().RtvWidth * Config::GetSystem().RtvHeight * NUM_RESERVOIR_SUB_BUFFERS;
    m_numContexts = Config::GetSystem().RtvWidth * Config::GetSystem().RtvHeight * NUM_CONTEXT_SUB_BUFFERS;

    const size_t reservoirBufferSizeDI = m_numReservoirs * sizeof(ReservoirDI);
    m_reservoirBufferDi.Init_Buffer("Reservoir DI", device, reservoirBufferSizeDI, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);

    const size_t reservoirBufferSizeGI = m_numReservoirs * sizeof(ReservoirGI);
    m_reservoirBufferGi.Init_Buffer("Reservoir GI", device, reservoirBufferSizeGI, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);

    const size_t contextBufferSize = m_numContexts * sizeof(ContextPrimaryRay);
    m_contextsPrimaryRay.Init_Buffer("ReSTIR Contexts", device, contextBufferSize, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);

    m_initialisedResources = true;
}

void RestirManager::Init(ID3D12Device* device, const RootSig* rootSig, const std::vector<std::string>& compileArgs)
{
    m_pipelineInitContext.InitCompute(device, "Compute/ReSTIR/InitContextCS.hlsl", rootSig->Get(), compileArgs);
    m_pipelineClear.InitCompute(device, "Compute/ReSTIR/ClearCS.hlsl", rootSig->Get(), compileArgs);

    m_pipelineDiGen.InitCompute(device, "Compute/ReSTIR/DI/GenerateSamplesCS.hlsl", rootSig->Get(), compileArgs);
    m_pipelineDiTemporal.InitCompute(device, "Compute/ReSTIR/DI/TemporalMergeCS.hlsl", rootSig->Get(), compileArgs);
    m_pipelineDiSpatio.InitCompute(device, "Compute/ReSTIR/DI/SpatialSampleCS.hlsl", rootSig->Get(), compileArgs);

    m_pipelineGiGen.InitCompute(device, "Compute/ReSTIR/GI/GenerateSamplesCS.hlsl", rootSig->Get(), compileArgs);
    m_pipelineGiTemporal.InitCompute(device, "Compute/ReSTIR/GI/TemporalMergeCS.hlsl", rootSig->Get(), compileArgs);
    m_pipelineGiSpatio.InitCompute(device, "Compute/ReSTIR/GI/SpatialSampleCS.hlsl", rootSig->Get(), compileArgs);

    if (!m_initialisedResources)
    {
        initResources(device);
    }
}

void RestirManager::PrePasses(ID3D12Device* device, ID3D12GraphicsCommandList* cmdList, const RestirConfig& config)
{
    GPU_SCOPE(cmdList, "ReSTIR DI: Generate Samples");

    if (!m_initialisedResources)
    {
        initResources(device);
    }

    if (!m_pipelineDiGen.GetPSO())
        return;

    m_reservoirBufferDi.Transition(cmdList, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
    m_contextsPrimaryRay.Transition(cmdList, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);

    // Clear Pass
    if (config.ClearBuffers)
    {
        cmdList->SetPipelineState(m_pipelineClear.GetPSO());
        DispatchOverTexture(cmdList, 16, Config::GetSystem().RtvWidth, Config::GetSystem().RtvHeight);

        m_reservoirBufferDi.UavBarrier(cmdList);
    }

    // Init Context
    {
        cmdList->SetPipelineState(m_pipelineInitContext.GetPSO());
        DispatchOverTexture(cmdList, 16, Config::GetSystem().RtvWidth, Config::GetSystem().RtvHeight);

        m_contextsPrimaryRay.UavBarrier(cmdList);
    }

    // DI
    if (config.EnabledDI)
    {
        // Generate Candidates Pass
        {
            cmdList->SetPipelineState(m_pipelineDiGen.GetPSO());
            DispatchOverTexture(cmdList, 16, Config::GetSystem().RtvWidth, Config::GetSystem().RtvHeight);

            m_reservoirBufferDi.UavBarrier(cmdList);
        }

        // Temporal Merge Pass
        if (config.EnabledTemporal)
        {
            cmdList->SetPipelineState(m_pipelineDiTemporal.GetPSO());
            DispatchOverTexture(cmdList, 16, Config::GetSystem().RtvWidth, Config::GetSystem().RtvHeight);

            m_reservoirBufferDi.UavBarrier(cmdList);
        }

        // Spatial Merge Pass
        if (config.EnabledSpatial)
        {
            cmdList->SetPipelineState(m_pipelineDiSpatio.GetPSO());
            DispatchOverTexture(cmdList, 16, Config::GetSystem().RtvWidth, Config::GetSystem().RtvHeight);

            m_reservoirBufferDi.UavBarrier(cmdList);
        }
    }

    // GI
    if (config.EnabledGI)
    {
        // Generate Candidates Pass
        {
            cmdList->SetPipelineState(m_pipelineGiGen.GetPSO());
            DispatchOverTexture(cmdList, 16, Config::GetSystem().RtvWidth, Config::GetSystem().RtvHeight);

            m_reservoirBufferGi.UavBarrier(cmdList);
        }

        // Temporal Merge Pass
        if (config.EnabledTemporal)
        {
            cmdList->SetPipelineState(m_pipelineGiTemporal.GetPSO());
            DispatchOverTexture(cmdList, 16, Config::GetSystem().RtvWidth, Config::GetSystem().RtvHeight);

            m_reservoirBufferGi.UavBarrier(cmdList);
        }

        // Spatial Merge Pass
        if (config.EnabledSpatial)
        {
            cmdList->SetPipelineState(m_pipelineGiSpatio.GetPSO());
            DispatchOverTexture(cmdList, 16, Config::GetSystem().RtvWidth, Config::GetSystem().RtvHeight);

            m_reservoirBufferGi.UavBarrier(cmdList);
        }
    }
}

void RestirManager::PostUpdate(D3D* d3d)
{

}
