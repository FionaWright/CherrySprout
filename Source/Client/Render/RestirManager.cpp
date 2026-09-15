#include "System/pch.h"

#include "Render/RestirManager.h"

#include "Debug/GPUEventScoped.h"
#include "HWI/D3D.h"
#include "PathTracing/ReSTIR/ReSTIR_DI_Structs.h"
#include "System/Config.h"
#include "Utils/CBVs.h"
#include "Utils/D3DUtils.h"

void RestirManager::Init(ID3D12Device* device, const RootSig* rootSig, const std::vector<std::string>& compileArgs)
{
    m_pipelineDiGenerateSamples.InitCompute(device, "Compute/ReSTIR/DI/GenerateSamplesCS.hlsl", rootSig->Get(), compileArgs);
}

void RestirManager::GenerateSamplesDi(ID3D12Device* device, ID3D12GraphicsCommandList* cmdList)
{
    GPU_SCOPE(cmdList, "ReSTIR DI: Generate Samples");

    if (!m_reservoirBuffer.GetResource())
    {
        m_numReservoirs = Config::GetSystem().RtvWidth * Config::GetSystem().RtvHeight * 2;
        const size_t reservoirBufferSize = m_numReservoirs * sizeof(ReservoirDI);
        m_reservoirBuffer.Init_Buffer("Reservoir", device, reservoirBufferSize, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);
    }

    if (!m_pipelineDiGenerateSamples.GetPSO())
        return;

    m_reservoirBuffer.Transition(cmdList, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);

    cmdList->SetPipelineState(m_pipelineDiGenerateSamples.GetPSO());

    DispatchOverTexture(cmdList, 16, Config::GetSystem().RtvWidth, Config::GetSystem().RtvHeight);

    D3D12_RESOURCE_BARRIER barrier = {};
    barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_UAV;
    barrier.UAV.pResource = m_reservoirBuffer.GetResource();
    cmdList->ResourceBarrier(1, &barrier);
}
