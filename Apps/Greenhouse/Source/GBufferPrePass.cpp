#include "System/pch.h"

#include "GBufferPrePass.h"

#include "Debug/GPUEventScoped.h"
#include "HWI/D3D.h"
#include "HWI/Heap.h"
#include "Scene/InstanceData.h"
#include "Scene/Material.h"
#include "Scene/Scene.h"
#include "Scene/Vertex.h"
#include "Utils/CBVs.h"
#include "Utils/D3DUtils.h"
#include "Utils/Helper.h"

void GBufferPrePass::Init(const D3D* d3d, Heap* heap, UploadHeap* uploadHeapCBV)
{
    const size_t w = Config::GetSystem().RtvWidth;
    const size_t h = Config::GetSystem().RtvHeight;

    m_rtvClearValues = {
        XMFLOAT4(0, 0, 0, 0),
        XMFLOAT4(1, 1, 1, 1),
        XMFLOAT4(0, 0, 0, 0),
    };
    m_gbufferTexMaterialIdx.Init_Tex2D("GBuffer Material Idx", d3d->GetDevice(), w, h, 1, GBUFFER_FORMAT_MAT_IDX, D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET, D3D12_RESOURCE_STATE_RENDER_TARGET, &m_rtvClearValues[0]);
    m_gbufferTexNormals.Init_Tex2D("GBuffer Normals", d3d->GetDevice(), w, h, 1, GBUFFER_FORMAT_NORMALS, D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET, D3D12_RESOURCE_STATE_RENDER_TARGET, &m_rtvClearValues[1]);
    m_gbufferTexUvMv.Init_Tex2D("GBuffer UV + MV", d3d->GetDevice(), w, h, 1, GBUFFER_FORMAT_UV_MV, D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET, D3D12_RESOURCE_STATE_RENDER_TARGET, &m_rtvClearValues[2]);
    m_gbufferTexDepth.Init_Depth2D("GBuffer Depth", d3d->GetDevice(), w, h, D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL, D3D12_RESOURCE_STATE_DEPTH_WRITE);

#define NUM_RENDER_TARGETS 3

    m_heapRTV.Init("GBuffer Pre-Pass Heap RTV", d3d->GetDevice(), NUM_RENDER_TARGETS, 0, D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
    m_heapIdxMatIdx = CreateRTV(d3d->GetDevice(), &m_heapRTV, &m_gbufferTexMaterialIdx);
    m_heapIdxNormals = CreateRTV(d3d->GetDevice(), &m_heapRTV, &m_gbufferTexNormals);
    m_heapIdxUvMv = CreateRTV(d3d->GetDevice(), &m_heapRTV, &m_gbufferTexUvMv);

    m_heapDSV.Init("GBuffer Pre-Pass Heap DSV", d3d->GetDevice(), 1, 0, D3D12_DESCRIPTOR_HEAP_TYPE_DSV);
    m_heapIdxDepth = CreateDSV(d3d->GetDevice(), &m_heapDSV, &m_gbufferTexDepth, GBUFFER_FORMAT_DEPTH);

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

    auto desc = CreateGraphicsPipelineDesc(m_rootSig.Get(), {ildDesc, _countof(ildDesc)}, true);
    desc.RasterizerState.CullMode = D3D12_CULL_MODE_BACK;
    desc.NumRenderTargets = NUM_RENDER_TARGETS;
    desc.RTVFormats[0] = GBUFFER_FORMAT_MAT_IDX;
    desc.RTVFormats[1] = GBUFFER_FORMAT_NORMALS;
    desc.RTVFormats[2] = GBUFFER_FORMAT_UV_MV;
    m_pipeline.InitGraphics(d3d->GetDevice(), "Raster/GBufferPrePassVS.hlsl", "Raster/GBufferPrePassPS.hlsl", desc);
}

void GBufferPrePass::LoadSceneData(D3D* d3d, Scene* scene)
{
    CherryAssert(scene->GPU.MegaBufferInstanceData.GetResource());
    CherryAssert(scene->GPU.MegaBufferMaterials.GetResource());

    m_descriptorSet.SetSRV_Buffer(d3d->GetDevice(), 0, &scene->GPU.MegaBufferInstanceData, scene->CPU.ObjectCount, sizeof(InstanceData));
    m_descriptorSet.SetSRV_Buffer(d3d->GetDevice(), 1, &scene->GPU.MegaBufferMaterials, scene->CPU.MegaBufferMaterialsCount, sizeof(Material));
}

void GBufferPrePass::Render(D3D* d3d, ID3D12GraphicsCommandList* cmdList, Scene* scene, const Heap* heap, const XMMATRIX& V, const XMMATRIX& P)
{
    GPU_SCOPE(cmdList, "GBuffer Pre-Pass");

    {
        CD3DX12_RECT scissorRect = SetViewportScissor(cmdList, Config::GetSystem().RtvWidth, Config::GetSystem().RtvHeight);

        m_gbufferTexMaterialIdx.Transition(cmdList, D3D12_RESOURCE_STATE_RENDER_TARGET);
        m_gbufferTexNormals.Transition(cmdList, D3D12_RESOURCE_STATE_RENDER_TARGET);
        m_gbufferTexUvMv.Transition(cmdList, D3D12_RESOURCE_STATE_RENDER_TARGET);
        m_gbufferTexDepth.Transition(cmdList, D3D12_RESOURCE_STATE_DEPTH_WRITE);

        D3D12_CPU_DESCRIPTOR_HANDLE rtvs[] = {
            m_heapRTV.GetDescriptorHandleAtIndex(m_heapIdxMatIdx),
            m_heapRTV.GetDescriptorHandleAtIndex(m_heapIdxNormals),
            m_heapRTV.GetDescriptorHandleAtIndex(m_heapIdxUvMv),
        };

        const CD3DX12_CPU_DESCRIPTOR_HANDLE dsvHandle = m_heapDSV.GetDescriptorHandleAtIndex(m_heapIdxDepth);
        cmdList->OMSetRenderTargets(_countof(rtvs), rtvs, FALSE, &dsvHandle);

        for (int i = 0; i < _countof(rtvs); i++)
        {
            cmdList->ClearRenderTargetView(rtvs[i], reinterpret_cast<FLOAT*>(&m_rtvClearValues[i]), 1, &scissorRect);
        }

        cmdList->ClearDepthStencilView(dsvHandle, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);
    }

    scene->GPU.MegaBufferInstanceData.Transition(cmdList, D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE);
    scene->GPU.MegaBufferMaterials.Transition(cmdList, D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE);

    {
        D3D12_VERTEX_BUFFER_VIEW viewV;
        D3D12_INDEX_BUFFER_VIEW viewI;
        VertexIndexBuffersToViews(&scene->GPU.MegaBufferVertex, &scene->GPU.MegaBufferIndex,
                                  scene->CPU.MegaBufferVertexCount, sizeof(Vertex), scene->CPU.MegaBufferIndexCount,
                                  viewV, viewI);

        scene->GPU.MegaBufferVertex.Transition(cmdList, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);
        scene->GPU.MegaBufferIndex.Transition(cmdList, D3D12_RESOURCE_STATE_INDEX_BUFFER);
        m_descriptorSet.TransitionAllSRVToShaderResource(cmdList);

        cmdList->IASetVertexBuffers(0, 1, &viewV);
        cmdList->IASetIndexBuffer(&viewI);
        cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

        cmdList->SetGraphicsRootSignature(m_rootSig.Get());
        cmdList->SetPipelineState(m_pipeline.GetPSO());
        heap->Bind(cmdList);
        heap->BindSceneTextures_Graphics(cmdList, m_rootSig.GetParamIndexSceneTextures());
        m_descriptorSet.SetDescriptorTables_Graphics(cmdList);
    }

    CbvMatrices_VP matricesVP = {};
    XMStoreFloat4x4(&matricesVP.V, V);
    XMStoreFloat4x4(&matricesVP.P, P);
    m_descriptorSet.UpdateCBV(0, &matricesVP);

    CbvGBufferPrePass_PerInstance perInstance{};

    for (int i = 0; i < scene->CPU.ObjectCount; ++i)
    {
        const Object& obj = scene->CPU.Objects[i];

        perInstance.InstanceIdx = i;
        m_rootConstants.Bind_Graphics(cmdList, &perInstance);

        cmdList->DrawIndexedInstanced(obj.MegaBufferIndexCount, 1, obj.MegaBufferIndexOffset,
                                      obj.MegaBufferVertexOffset, 0);
    }
}

void GBufferPrePass::UnreserveData()
{
    m_gbufferTexMaterialIdx.Release();
    m_gbufferTexNormals.Release();
    m_gbufferTexUvMv.Release();
    m_gbufferTexDepth.Release();
}
