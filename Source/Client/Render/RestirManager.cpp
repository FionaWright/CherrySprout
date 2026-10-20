#include "System/pch.h"

#include "Render/RestirManager.h"

#include "Debug/GPUEventScoped.h"
#include "HWI/D3D.h"
#include "PathTracing/ReSTIR/Structs.h"
#include "System/Config.h"
#include "Utils/CBVs.h"
#include "Utils/D3DUtils.h"

#define DEBUG_MODE_READBACK 0

#define NUM_RESERVOIR_SUB_BUFFERS 3 // Ping, Pong, Prev
#define NUM_CONTEXT_SUB_BUFFERS 2 // Curr, Prev

void RestirManager::initResources(ID3D12Device* device)
{
    m_numReservoirs = Config::GetSystem().RtvWidth * Config::GetSystem().RtvHeight * NUM_RESERVOIR_SUB_BUFFERS;
    m_numContexts = Config::GetSystem().RtvWidth * Config::GetSystem().RtvHeight * NUM_CONTEXT_SUB_BUFFERS;

    const size_t reservoirBufferSizeDI = m_numReservoirs * sizeof(ReservoirDI);
    m_reservoirBufferDiRW.Init_Buffer("Reservoir DI (RW)", device, reservoirBufferSizeDI, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);
    m_reservoirBufferDiReadback.Init_Buffer("Reservoir DI (Readback)", device, reservoirBufferSizeDI, D3D12_RESOURCE_FLAG_NONE, true);

    const size_t reservoirBufferSizeGI = m_numReservoirs * sizeof(ReservoirGI);
    m_reservoirBufferGi.Init_Buffer("Reservoir GI (RW)", device, reservoirBufferSizeGI, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);

    const size_t contextBufferSize = m_numContexts * sizeof(ContextPrimaryRay);
    m_contextsPrimaryRay.Init_Buffer("ReSTIR Contexts", device, contextBufferSize, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);

    m_initialisedResources = true;
}

void RestirManager::Init(ID3D12Device* device, const RootSig* rootSig, const std::vector<std::string>& compileArgs)
{
    m_pipelineInitContext.InitCompute(device, "Compute/ReSTIR/InitContextCS.hlsl", rootSig->Get(), compileArgs);
    m_pipelineDiGenerateSamples.InitCompute(device, "Compute/ReSTIR/DI/GenerateSamplesCS.hlsl", rootSig->Get(), compileArgs);
    m_pipelineDiTemporalMerge.InitCompute(device, "Compute/ReSTIR/DI/TemporalMergeCS.hlsl", rootSig->Get(), compileArgs);
    m_pipelineDiSpatialSample.InitCompute(device, "Compute/ReSTIR/DI/SpatialSampleCS.hlsl", rootSig->Get(), compileArgs);
    m_pipelineDiClear.InitCompute(device, "Compute/ReSTIR/DI/ClearCS.hlsl", rootSig->Get(), compileArgs);

    if (!m_initialisedResources)
    {
        initResources(device);
    }
}

void RestirManager::PrePassesDI(ID3D12Device* device, ID3D12GraphicsCommandList* cmdList, bool isTemporalEnabled, bool isSpatialEnabled, bool clear)
{
    GPU_SCOPE(cmdList, "ReSTIR DI: Generate Samples");

    if (!m_initialisedResources)
    {
        initResources(device);
    }

    if (!m_pipelineDiGenerateSamples.GetPSO())
        return;

    m_reservoirBufferDiRW.Transition(cmdList, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
    m_contextsPrimaryRay.Transition(cmdList, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);

    // Clear Pass
    if (clear)
    {
        cmdList->SetPipelineState(m_pipelineDiClear.GetPSO());
        DispatchOverTexture(cmdList, 16, Config::GetSystem().RtvWidth, Config::GetSystem().RtvHeight);

        m_reservoirBufferDiRW.UavBarrier(cmdList);
    }

    // Init Context
    {
        cmdList->SetPipelineState(m_pipelineInitContext.GetPSO());
        DispatchOverTexture(cmdList, 16, Config::GetSystem().RtvWidth, Config::GetSystem().RtvHeight);

        m_contextsPrimaryRay.UavBarrier(cmdList);
    }

    // Generate Candidates Pass
    {
        cmdList->SetPipelineState(m_pipelineDiGenerateSamples.GetPSO());
        DispatchOverTexture(cmdList, 16, Config::GetSystem().RtvWidth, Config::GetSystem().RtvHeight);

        m_reservoirBufferDiRW.UavBarrier(cmdList);
    }

    // Temporal Merge Pass
    if (isTemporalEnabled)
    {
        cmdList->SetPipelineState(m_pipelineDiTemporalMerge.GetPSO());
        DispatchOverTexture(cmdList, 16, Config::GetSystem().RtvWidth, Config::GetSystem().RtvHeight);

        m_reservoirBufferDiRW.UavBarrier(cmdList);
        m_contextsPrimaryRay.UavBarrier(cmdList);
    }

    // Spatial Merge Pass
    if (isSpatialEnabled)
    {
        cmdList->SetPipelineState(m_pipelineDiSpatialSample.GetPSO());
        DispatchOverTexture(cmdList, 16, Config::GetSystem().RtvWidth, Config::GetSystem().RtvHeight);

        m_reservoirBufferDiRW.UavBarrier(cmdList);
    }

#if DEBUG_MODE_READBACK
    m_reservoirBufferDiRW.Transition(cmdList, D3D12_RESOURCE_STATE_COPY_SOURCE);
    m_reservoirBufferDiReadback.Transition(cmdList, D3D12_RESOURCE_STATE_COPY_DEST);
    cmdList->CopyBufferRegion(m_reservoirBufferDiReadback.GetResource(), 0, m_reservoirBufferDiRW.GetResource(), 0, m_numReservoirs * sizeof(ReservoirDI));
#endif
}

void RestirManager::PostUpdate(D3D* d3d)
{
#if DEBUG_MODE_READBACK
    d3d->Flush();

    m_cpuReservoirBufferReadback.resize(m_numReservoirs);

    m_reservoirBufferDiReadback.Readback(m_cpuReservoirBufferReadback.data());
#endif
}
