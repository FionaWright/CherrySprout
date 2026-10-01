#include "System/pch.h"
#include "Scene/TextureConverter.h"

#include "Debug/GPUEventScoped.h"
#include "Utils/D3DUtils.h"
#include "Utils/Helper.h"

void TextureConverter::Init(const D3D* d3d)
{
    m_rootConstants.Init(0, 0, sizeof(CbvTextureConvert));

    m_rootSig.SmartInit(d3d->GetDevice(), 0, 1, 2, false, nullptr, 0, &m_rootConstants);

    m_pipeline.InitCompute(d3d->GetDevice(), "Compute/TextureConvertCS.hlsl", m_rootSig.Get());
}

void TextureConverter::Convert(const D3D* d3d, Heap* heap, ID3D12GraphicsCommandList* cmdList, D12Resource* source, TextureConvertMode mode, D12Resource* dest4, D12Resource* dest1)
{
    dest4->Init_Tex2D("Normal Map", d3d->GetDevice(), source->GetDesc().Width, source->GetDesc().Height, 1, DXGI_FORMAT_R8G8B8A8_UNORM, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);

    source->Transition(cmdList, D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE);
    dest4->Transition(cmdList, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);

    heap->Bind(cmdList);

    cmdList->SetPipelineState(m_pipeline.GetPSO());
    cmdList->SetComputeRootSignature(m_rootSig.Get());

    CbvTextureConvert cbv{};
    cbv.Mode = mode;
    m_rootConstants.Bind_Compute(cmdList, &cbv);

    DescriptorSet set;
    set.Init(heap, false, true);
    set.SetSRV_Tex2D(d3d->GetDevice(), 0, source, source->GetDesc().Format);
    set.SetUAV_Tex2D(d3d->GetDevice(), 0, dest4, dest4->GetDesc().Format);
    set.TransitionAllSRVToShaderResource(cmdList);
    set.SetDescriptorTables_Compute(cmdList);

    DispatchOverTexture(cmdList, 16, source->GetDesc().Width, source->GetDesc().Height);
}
