#include "System/pch.h"

#include "Render/RestirManager.h"

#include "Debug/GPUEventScoped.h"
#include "HWI/D3D.h"
#include "PathTracing/ReSTIR/Structs.h"
#include "System/Config.h"
#include "Utils/D3DUtils.h"

#define NUM_RESERVOIR_SUB_BUFFERS 3 // Ping, Pong, Prev
#define NUM_CONTEXT_SUB_BUFFERS 2 // Curr, Prev

void RestirManager::Init(ID3D12Device* device, RootSig* rootSig, const std::vector<std::string>& compileArgs)
{
    m_rootSigCached = rootSig;
    m_compileArgsCached = compileArgs;

    if (!m_reservoirBufferDi.IsInitialized())
    {
        m_reservoirBufferDi.Init_Buffer("Reservoir DI", device, sizeof(ReservoirDI), D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);
        m_reservoirBufferGi.Init_Buffer("Reservoir GI", device, sizeof(ReservoirGI), D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);
        m_reservoirBufferPt.Init_Buffer("Reservoir PT", device, sizeof(ReservoirPT), D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);
        m_contextsPrimaryRay.Init_Buffer("ReSTIR Contexts", device, sizeof(ContextPrimaryRay), D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);

        m_numReservoirs = 1;
        m_numContexts = 1;
    }
}

void RestirManager::PrePasses(ID3D12Device* device, ID3D12GraphicsCommandList* cmdList, const RestirConfig& config)
{
    GPU_SCOPE(cmdList, "ReSTIR DI: Generate Samples");

    m_numReservoirs = Config::GetSystem().RtvWidth * Config::GetSystem().RtvHeight * NUM_RESERVOIR_SUB_BUFFERS;
    m_numContexts = Config::GetSystem().RtvWidth * Config::GetSystem().RtvHeight * NUM_CONTEXT_SUB_BUFFERS;

    const size_t reservoirBufferSizeDI = m_numReservoirs * sizeof(ReservoirDI);
    const size_t reservoirBufferSizeGI = m_numReservoirs * sizeof(ReservoirGI);
    const size_t reservoirBufferSizePT = m_numReservoirs * sizeof(ReservoirPT);
    const size_t contextBufferSize = m_numContexts * sizeof(ContextPrimaryRay);

    if (m_contextsPrimaryRay.GetDesc().Width != contextBufferSize)
    {
        m_contextsPrimaryRay.Reset();
        m_contextsPrimaryRay.Init_Buffer("ReSTIR Contexts", device, contextBufferSize, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);
    }
    if (config.EnabledDI && m_reservoirBufferDi.GetDesc().Width != reservoirBufferSizeDI)
    {
        m_reservoirBufferDi.Reset();
        m_reservoirBufferDi.Init_Buffer("Reservoir DI", device, reservoirBufferSizeDI, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);
    }
    if (config.EnabledGI && m_reservoirBufferGi.GetDesc().Width != reservoirBufferSizeGI)
    {
        m_reservoirBufferGi.Reset();
        m_reservoirBufferGi.Init_Buffer("Reservoir GI", device, reservoirBufferSizeGI, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);
    }
    if (config.EnabledPT && m_reservoirBufferPt.GetDesc().Width != reservoirBufferSizePT)
    {
        m_reservoirBufferPt.Reset();
        m_reservoirBufferPt.Init_Buffer("Reservoir PT", device, reservoirBufferSizePT, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);
    }

    m_contextsPrimaryRay.Transition(cmdList, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
    if (config.EnabledDI)
        m_reservoirBufferDi.Transition(cmdList, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
    if (config.EnabledGI)
        m_reservoirBufferGi.Transition(cmdList, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
    if (config.EnabledPT)
        m_reservoirBufferPt.Transition(cmdList, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);

    if (!m_pipelineInitContext.GetPSO())
    {
        V(m_pipelineInitContext.InitCompute(device, "Compute/ReSTIR/InitContextCS.hlsl", m_rootSigCached->Get(), m_compileArgsCached));
        V(m_pipelineClear.InitCompute(device, "Compute/ReSTIR/ClearCS.hlsl", m_rootSigCached->Get(), m_compileArgsCached));
    }

    auto dispatchRestirPass = [&](const Pipeline* pipeline, const D12Resource* resource = nullptr)
    {
        cmdList->SetPipelineState(pipeline->GetPSO());
        DispatchOverTexture(cmdList, 16, Config::GetSystem().RtvWidth, Config::GetSystem().RtvHeight);

        if (resource)
            resource->UavBarrier(cmdList);
    };

    // Clear Pass
    if (config.ClearBuffers)
        dispatchRestirPass(&m_pipelineClear);

    // Init Context
    {
        dispatchRestirPass(&m_pipelineInitContext, &m_contextsPrimaryRay);
    }

    // DI
    if (config.EnabledDI)
    {
        if (!m_pipelineDiGen.GetPSO())
        {
            V(m_pipelineDiGen.InitCompute(device, "Compute/ReSTIR/DI/GenerateSamplesCS.hlsl", m_rootSigCached->Get(), m_compileArgsCached));
            V(m_pipelineDiTemporal.InitCompute(device, "Compute/ReSTIR/DI/TemporalReuseCS.hlsl", m_rootSigCached->Get(), m_compileArgsCached));
            V(m_pipelineDiSpatio.InitCompute(device, "Compute/ReSTIR/DI/SpatialReuseCS.hlsl", m_rootSigCached->Get(), m_compileArgsCached));
        }

        // Generate Candidates Pass
        {
            m_reservoirBufferDi.UavBarrier(cmdList);
            dispatchRestirPass(&m_pipelineDiGen, &m_reservoirBufferDi);
        }

        // Temporal Merge Pass
        if (config.EnabledTemporal)
            dispatchRestirPass(&m_pipelineDiTemporal, &m_reservoirBufferDi);

        // Spatial Merge Pass
        if (config.EnabledSpatial)
            dispatchRestirPass(&m_pipelineDiSpatio, &m_reservoirBufferDi);
    }

    // GI
    if (config.EnabledGI)
    {
        if (!m_pipelineGiGen.GetPSO())
        {
            V(m_pipelineGiGen.InitCompute(device, "Compute/ReSTIR/GI/GenerateSamplesCS.hlsl", m_rootSigCached->Get(), m_compileArgsCached));
            V(m_pipelineGiTemporal.InitCompute(device, "Compute/ReSTIR/GI/TemporalReuseCS.hlsl", m_rootSigCached->Get(), m_compileArgsCached));
            V(m_pipelineGiSpatio.InitCompute(device, "Compute/ReSTIR/GI/SpatialReuseCS.hlsl", m_rootSigCached->Get(), m_compileArgsCached));
        }

        // Generate Candidates Pass
        {
            m_reservoirBufferGi.UavBarrier(cmdList);
            dispatchRestirPass(&m_pipelineGiGen, &m_reservoirBufferGi);
        }

        // Temporal Merge Pass
        if (config.EnabledTemporal)
            dispatchRestirPass(&m_pipelineGiTemporal, &m_reservoirBufferGi);

        // Spatial Merge Pass
        if (config.EnabledSpatial)
            dispatchRestirPass(&m_pipelineGiSpatio, &m_reservoirBufferGi);
    }

    // PT
    if (config.EnabledPT)
    {
        if (!m_pipelinePtGen.GetPSO())
        {
            V(m_pipelinePtGen.InitCompute(device, "Compute/ReSTIR/PT/GenerateSamplesCS.hlsl", m_rootSigCached->Get(), m_compileArgsCached));
            V(m_pipelinePtTemporal.InitCompute(device, "Compute/ReSTIR/PT/TemporalReuseCS.hlsl", m_rootSigCached->Get(), m_compileArgsCached));
            V(m_pipelinePtSpatio.InitCompute(device, "Compute/ReSTIR/PT/SpatialReuseCS.hlsl", m_rootSigCached->Get(), m_compileArgsCached));
        }

        // Generate Candidates Pass
        {
            m_reservoirBufferPt.UavBarrier(cmdList);
            dispatchRestirPass(&m_pipelinePtGen, &m_reservoirBufferPt);
        }

        // Temporal Merge Pass
        if (config.EnabledTemporal)
            dispatchRestirPass(&m_pipelinePtTemporal, &m_reservoirBufferPt);

        // Spatial Merge Pass
        if (config.EnabledSpatial)
            dispatchRestirPass(&m_pipelinePtSpatio, &m_reservoirBufferPt);
    }
}

void RestirManager::PostUpdate(D3D* d3d)
{

}
