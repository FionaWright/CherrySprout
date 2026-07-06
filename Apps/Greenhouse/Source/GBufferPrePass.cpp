#include "System/pch.h"

#include "GBufferPrePass.h"

#include "HWI/D3D.h"
#include "Scene/Material.h"
#include "Utils/CBVs.h"
#include "Utils/D3DUtils.h"

void GBufferPrePass::Init(D3D* d3d, Heap* heap, UploadHeap* uploadHeapCBV)
{
    const size_t w = Config::GetSystem().RtvWidth;
    const size_t h = Config::GetSystem().RtvHeight;

    m_gbufferTexMaterialIdx.Init_Tex2D("GBuffer Material Idx", d3d->GetDevice(), w, h, 1, DXGI_FORMAT_R8_UINT);
    m_gbufferTexNormals.Init_Tex2D("GBuffer Normals", d3d->GetDevice(), w, h, 1, DXGI_FORMAT_R10G10B10A2_UINT);
    m_gbufferTexDepth.Init_Tex2D("GBuffer Depth", d3d->GetDevice(), w, h, 1, DXGI_FORMAT_D16_UNORM); // Use D32F ? Worried it might cause sneaky bugs later
    m_gbufferTexUvMv.Init_Tex2D("GBuffer UV + MV", d3d->GetDevice(), w, h, 1, DXGI_FORMAT_R16G16B16A16_FLOAT);

    D3D12_STATIC_SAMPLER_DESC sampler = {};
    InitializeSamplerLinearClamp(&sampler);

    m_rootConstants.Init(0, 0, sizeof(CbvGBufferPrePass_PerInstance));
    m_rootSig.SmartInit(d3d->GetDevice(), 1, 2, 0, true, &sampler, 1, &m_rootConstants);

    m_descriptorSet.Init(heap, true, true);
    m_descriptorSet.AddCBV(d3d->GetDevice(), sizeof(CbvMatrices_VP), uploadHeapCBV);

    D3D12_INPUT_ELEMENT_DESC ildDesc[] =
    {
        {
            "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT,
            D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0
        },
        {
            "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT,
            D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0
        },
        {
            "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT,
            D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0
        },
    };

    auto desc = CreateGraphicsPipelineDesc(m_rootSig.Get(), { ildDesc, _countof(ildDesc) }, true);
    desc.RasterizerState.CullMode = D3D12_CULL_MODE_BACK;
    m_pipeline.InitGraphics(d3d->GetDevice(), "Raster/GBufferPrePassVS.hlsl", "Raster/GBufferPrePassPS.hlsl", desc);
}

void GBufferPrePass::Render(D3D* d3d, ID3D12GraphicsCommandList* cmdList, Scene* scene)
{

}
