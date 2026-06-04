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

    m_rootSig.SmartInit(d3d->GetDevice(), 3, 0, 0, false, &sampler, 1);

    m_descriptorSet.Init(heap, false);
    m_descriptorSet.AddCBV(d3d->GetDevice(), sizeof(CbvMatrices), uploadHeapCBV);
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

    m_shader.InitVsPs("Raster/ForwardVS.hlsl", "Raster/ForwardPS.hlsl", {ildDesc, _countof(ildDesc)}, d3d->GetDevice(),
                      m_rootSig.Get(), true);
}

void Forward::Update(D3D* d3d, TimeArgs timeArgs)
{
}

void Forward::Render(D3D* d3d, ID3D12GraphicsCommandList* cmdList, const GreenHouseRenderInfo& renderInfo)
{
    GPU_SCOPE(cmdList, "Forward Backend");

    Scene* scene = renderInfo.Scene;
    Heap* heap = renderInfo.Heap;

    {
        const uint32_t w = Config::GetSystem().RtvWidth;
        const uint32_t h = Config::GetSystem().RtvHeight;
        const uint32_t left = Config::GetSystem().WindowAppGuiWidth;
        const CD3DX12_VIEWPORT viewport(float(left), 0.0f, float(w), float(h));
        const CD3DX12_RECT scissorRect(left, 0, left + w, h);

        cmdList->RSSetViewports(1, &viewport);
        cmdList->RSSetScissorRects(1, &scissorRect);

        const auto rtvHandle = d3d->GetRtvHandle();
        const CD3DX12_CPU_DESCRIPTOR_HANDLE dsvHandle(d3d->GetDsvHeapStart(), 0, d3d->GetDsvDescriptorSize());
        cmdList->OMSetRenderTargets(1, &rtvHandle, FALSE, &dsvHandle);

        cmdList->ClearRenderTargetView(rtvHandle, Config::GetRender().RtvClearColor, 1, &scissorRect);
        cmdList->ClearDepthStencilView(dsvHandle, D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.0f, 0, 0, nullptr);
    }

    //const CD3DX12_GPU_DESCRIPTOR_HANDLE bindlessHandle(heap->GetGPUHandle(), heap->GetBindlessTexBase(), heap->GetIncrementSize());

    {
        D3D12_VERTEX_BUFFER_VIEW viewV;
        D3D12_INDEX_BUFFER_VIEW viewI;
        VertexIndexBuffersToViews(&scene->GPU.MegaBufferVertex, &scene->GPU.MegaBufferIndex, scene->CPU.MegaBufferVertex.size(), sizeof(Vertex), scene->CPU.MegaBufferIndex.size(), viewV, viewI);

        scene->GPU.MegaBufferVertex.Transition(cmdList, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);
        scene->GPU.MegaBufferIndex.Transition(cmdList, D3D12_RESOURCE_STATE_INDEX_BUFFER);
        m_descriptorSet.TransitionAllSRVToShaderResource(cmdList);

        cmdList->IASetVertexBuffers(0, 1, &viewV);
        cmdList->IASetIndexBuffer(&viewI);
        cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

        heap->Bind(cmdList);
        cmdList->SetGraphicsRootSignature(m_rootSig.Get());
        cmdList->SetPipelineState(m_shader.GetPSO());
    }

    {
        //GPU_SCOPE(cmdList, "Skybox Pass");

        //if (skybox)
        //    skybox->RenderForward(d3d, cmdList, vMatrix, pMatrix);
    }

    CbvMatrices matrices = {};
    XMStoreFloat4x4(&matrices.V, *renderInfo.V);
    XMStoreFloat4x4(&matrices.P, *renderInfo.P);

    for (int i = 0; i < scene->CPU.Objects.size(); ++i)
    {
        const Object& obj = scene->CPU.Objects[i];

        GPU_SCOPE(cmdList, obj.DebugName.c_str());

        const XMMATRIX M = XMMatrixSet(
            obj.M[0], obj.M[1], obj.M[2], obj.M[3],
            obj.M[4], obj.M[5], obj.M[6], obj.M[7],
            obj.M[8], obj.M[9], obj.M[10], obj.M[11],
            obj.M[12], obj.M[13], obj.M[14], obj.M[15]
            );

        XMStoreFloat4x4(&matrices.M, M);
        XMStoreFloat4x4(&matrices.MTI, XMMatrixTranspose(XMMatrixInverse(nullptr, M)));
        m_descriptorSet.UpdateCBV(0, &matrices);

        m_descriptorSet.SetDescriptorTables_Graphics(cmdList);
        //cmdList->SetGraphicsRootDescriptorTable(2, bindlessHandle);

        cmdList->DrawIndexedInstanced(obj.MegaBufferIndexCount, 1, obj.MegaBufferIndexOffset, obj.MegaBufferVertexOffset, 0);
    }
}
