#include "System/pch.h"

#include "Render/RestirManager.h"

#include "Debug/GPUEventScoped.h"
#include "HWI/D3D.h"
#include "PathTracing/ReSTIR/ReSTIR_DI_Structs.h"
#include "System/Config.h"
#include "Utils/CBVs.h"
#include "Utils/D3DUtils.h"

#include "PathTracing/Flags/Flags.h"

void RestirManager::Init(D3D* d3d, const RootSig* rootSig)
{
    m_numReservoirs = Config::GetSystem().RtvWidth * Config::GetSystem().RtvHeight * 2;
    const size_t reservoirBufferSize = m_numReservoirs * sizeof(ReservoirDI);
    m_reservoirBuffer.Init_Buffer("Reservoir", d3d->GetDevice(), reservoirBufferSize, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);

    std::vector<std::string> compileArgs = {};
    for (int i = 0; i < FEATURE_COUNT; i++)
    {
        const hlsl::uint flagValue = 1u << i;
        compileArgs.emplace_back("-DFEATURE_FLAG_VALUE_" + std::string(s_featureFlagNames[i]) + "=" + std::to_string(flagValue));
    }
    for (int i = 0; i < DEBUG_COUNT; i++)
    {
        const hlsl::uint flagValue = 1u << i;
        compileArgs.emplace_back("-DDEBUG_FLAG_VALUE_" + std::string(s_debugFlagNames[i]) + "=" + std::to_string(flagValue));
    }
    // TODO: PT Settings

    m_pipelineDiGenerateSamples.InitCompute(d3d->GetDevice(), "Compute/ReSTIR/DI/GenerateSamplesCS.hlsl", rootSig->Get(), compileArgs);
}

void RestirManager::GenerateSamplesDi(ID3D12GraphicsCommandList* cmdList, const Heap* heap)
{
    GPU_SCOPE(cmdList, "ReSTIR DI: Generate Samples");

    m_reservoirBuffer.Transition(cmdList, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);

    heap->Bind(cmdList);
    cmdList->SetPipelineState(m_pipelineDiGenerateSamples.GetPSO());

    DispatchOverTexture(cmdList, 16, Config::GetSystem().RtvWidth, Config::GetSystem().RtvHeight);
}
