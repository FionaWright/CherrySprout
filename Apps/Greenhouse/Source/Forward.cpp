//
// Created by fiona on 01/06/2026.
//

#include "System/pch.h"
#include "Forward.h"

#include "Greenhouse.h"
#include "Debug/GPUEventScoped.h"
#include "HWI/Heap.h"
#include "Scene/Scene.h"
#include "System/HighResolutionClock.h"
#include "Utils/D3DUtils.h"

void Forward::Init(D3D* d3d, Heap* heap, UploadHeap* uploadHeapCBV)
{
    IRenderBackend::Init(d3d, heap, uploadHeapCBV);

    D3D12_STATIC_SAMPLER_DESC sampler = {};
    InitializeSamplerLinearClamp(&sampler);

    m_rootConstants.Init(0, 0, sizeof(CbvMatrices_M));

    m_rootSig.SmartInit(d3d->GetDevice(), 3, 0, 0, false, &sampler, 1, &m_rootConstants);

    m_descriptorSet.Init(heap, false, true);
    m_descriptorSet.AddCBV(d3d->GetDevice(), sizeof(CbvMatrices_VP), uploadHeapCBV);
    m_descriptorSet.AddCBV(d3d->GetDevice(), sizeof(CbvForward), uploadHeapCBV);
    m_descriptorSet.AddCBV(d3d->GetDevice(), sizeof(Material), uploadHeapCBV);

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
    m_pipeline.InitGraphics(d3d->GetDevice(), "Raster/ForwardVS.hlsl", "Raster/ForwardPS.hlsl", desc);
}

void Forward::LoadSceneData(D3D* d3d, Scene* scene, Heap* heap, UploadHeap* uploadHeapCBV, EnvironmentMap* envMap, LightImportanceSampler* lightImportanceSampler, GBufferPrePass* gbuffer)
{
    IRenderBackend::LoadSceneData(d3d, scene, heap, uploadHeapCBV, envMap, lightImportanceSampler, gbuffer);

    if (!envMap->GetCubemap()->IsInitialized())
    {
        envMap->InitCubemap(d3d, heap);
        m_skybox.Init(d3d, envMap->GetCubemap());
        m_skybox.UpdateDescriptorSet(d3d->GetDevice(), envMap->GetCubemap(), heap, uploadHeapCBV);
    }
}

void Forward::Update(D3D* d3d, Heap* heap, TimeArgs timeArgs)
{
}

void Forward::PostUpdate(D3D* d3d, const GreenHouseRenderInfo& renderInfo)
{

}

void Forward::Render(D3D* d3d, ID3D12GraphicsCommandList* cmdList, const GreenHouseRenderInfo& renderInfo, D12Resource* RTV, CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle)
{
    GPU_SCOPE(cmdList, "Forward Backend");

    Scene* scene = renderInfo.Scene;
    Heap* heap = renderInfo.Heap;

    {
        CD3DX12_RECT scissorRect = SetViewportScissor(cmdList, Config::GetSystem().RtvWidth, Config::GetSystem().RtvHeight, 0);

        RTV->Transition(cmdList, D3D12_RESOURCE_STATE_RENDER_TARGET);

        const CD3DX12_CPU_DESCRIPTOR_HANDLE dsvHandle(d3d->GetDsvHeapStart(), 0, d3d->GetDsvDescriptorSize());
        cmdList->OMSetRenderTargets(1, &rtvHandle, FALSE, &dsvHandle);

        cmdList->ClearDepthStencilView(dsvHandle, D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.0f, 0, 0, nullptr);
    }

    heap->Bind(cmdList);

    {
        GPU_SCOPE(cmdList, "Skybox Pass");
        m_skybox.RenderForward(cmdList, &renderInfo.V, &renderInfo.P);
    }

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
    }

    CbvMatrices_VP matricesVP = {};
    XMStoreFloat4x4(&matricesVP.V, renderInfo.V);
    XMStoreFloat4x4(&matricesVP.P, renderInfo.P);
    m_descriptorSet.UpdateCBV(0, &matricesVP);

    m_descriptorSet.SetDescriptorTables_Graphics(cmdList);
    //cmdList->SetGraphicsRootDescriptorTable(2, bindlessHandle);

    CbvMatrices_M matricesM = {};

    for (int i = 0; i < scene->CPU.ObjectCount; ++i)
    {
        const Object& obj = scene->CPU.Objects[i];

        //GPU_SCOPE(cmdList, obj.DebugName);

        const XMMATRIX M = XMMatrixSet(
            obj.M[0], obj.M[1], obj.M[2], obj.M[3],
            obj.M[4], obj.M[5], obj.M[6], obj.M[7],
            obj.M[8], obj.M[9], obj.M[10], obj.M[11],
            obj.M[12], obj.M[13], obj.M[14], obj.M[15]
            );

        XMStoreFloat4x4(&matricesM.M, M);
        XMStoreFloat4x4(&matricesM.MTI, XMMatrixTranspose(XMMatrixInverse(nullptr, M)));
        m_rootConstants.Bind_Graphics(cmdList, &matricesM);

        cmdList->DrawIndexedInstanced(obj.MegaBufferIndexCount, 1, obj.MegaBufferIndexOffset, obj.MegaBufferVertexOffset, 0);
    }
}
