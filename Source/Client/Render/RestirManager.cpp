#include "System/pch.h"

#include "Render/RestirManager.h"

#include "Debug/GPUEventScoped.h"
#include "HWI/D3D.h"
#include "PathTracing/ReSTIR/ReSTIR_DI_Structs.h"
#include "System/Config.h"
#include "Utils/CBVs.h"
#include "Utils/D3DUtils.h"

#define DEBUG_MODE_READBACK 0

void RestirManager::Init(ID3D12Device* device, const RootSig* rootSig, const std::vector<std::string>& compileArgs)
{
    m_pipelineDiGenerateSamples.InitCompute(device, "Compute/ReSTIR/DI/GenerateSamplesCS.hlsl", rootSig->Get(), compileArgs);

    if (!m_reservoirBufferRW.GetResource())
    {
        m_numReservoirs = Config::GetSystem().RtvWidth * Config::GetSystem().RtvHeight * 2;
        const size_t reservoirBufferSize = m_numReservoirs * sizeof(ReservoirDI);
        m_reservoirBufferRW.Init_Buffer("Reservoir (RW)", device, reservoirBufferSize, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);
        m_reservoirBufferReadback.Init_Buffer("Reservoir (Readback)", device, reservoirBufferSize, D3D12_RESOURCE_FLAG_NONE, true);
    }
}

void RestirManager::GenerateSamplesDi(ID3D12Device* device, ID3D12GraphicsCommandList* cmdList)
{
    GPU_SCOPE(cmdList, "ReSTIR DI: Generate Samples");

    if (!m_reservoirBufferRW.GetResource())
    {
        m_numReservoirs = Config::GetSystem().RtvWidth * Config::GetSystem().RtvHeight * 2;
        const size_t reservoirBufferSize = m_numReservoirs * sizeof(ReservoirDI);
        m_reservoirBufferRW.Init_Buffer("Reservoir (RW)", device, reservoirBufferSize, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);
        m_reservoirBufferReadback.Init_Buffer("Reservoir (Readback)", device, reservoirBufferSize, D3D12_RESOURCE_FLAG_NONE, true);
    }

    if (!m_pipelineDiGenerateSamples.GetPSO())
        return;

    m_reservoirBufferRW.Transition(cmdList, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);

    cmdList->SetPipelineState(m_pipelineDiGenerateSamples.GetPSO());

    DispatchOverTexture(cmdList, 16, Config::GetSystem().RtvWidth, Config::GetSystem().RtvHeight);

    D3D12_RESOURCE_BARRIER barrier = {};
    barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_UAV;
    barrier.UAV.pResource = m_reservoirBufferRW.GetResource();
    cmdList->ResourceBarrier(1, &barrier);

#if DEBUG_MODE_READBACK
    m_reservoirBufferRW.Transition(cmdList, D3D12_RESOURCE_STATE_COPY_SOURCE);
    m_reservoirBufferReadback.Transition(cmdList, D3D12_RESOURCE_STATE_COPY_DEST);
    cmdList->CopyBufferRegion(m_reservoirBufferReadback.GetResource(), 0, m_reservoirBufferRW.GetResource(), 0, m_numReservoirs * sizeof(ReservoirDI));
#endif
}

void RestirManager::PostUpdate(D3D* d3d)
{
#if DEBUG_MODE_READBACK
    d3d->Flush();

    m_cpuReservoirBufferReadback.resize(m_numReservoirs);

    m_reservoirBufferReadback.Readback(m_cpuReservoirBufferReadback.data());
#endif
}
