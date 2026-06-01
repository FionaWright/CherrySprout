//
// Created by fiona on 01/06/2026.
//

#include "System/pch.h"
#include "Forward.h"

#include "Debug/GPUEventScoped.h"
#include "HWI/Heap.h"
#include "Scene/Scene.h"
#include "System/HighResolutionClock.h"
#include "Utils/D3DUtils.h"

void Forward::Init(D3D* d3d, Heap* heap, UploadHeap* uploadHeapCBV, Scene* scene)
{
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

void Forward::Render(D3D* d3d, ID3D12GraphicsCommandList* cmdList, const Heap* heap, Scene* scene, const XMMATRIX& V, const XMMATRIX& P) const
{
    GPU_SCOPE(cmdList, "Forward Backend");

    const float fRtvWidth = static_cast<float>(Config::GetSystem().RtvWidth);
    const float fRtvHeight = static_cast<float>(Config::GetSystem().RtvHeight);

    const CD3DX12_VIEWPORT viewport(0.0f, 0.0f, fRtvWidth, fRtvHeight);
    const CD3DX12_RECT scissorRect(0, 0, Config::GetSystem().RtvWidth, Config::GetSystem().RtvHeight);

    cmdList->RSSetViewports(1, &viewport);
    cmdList->RSSetScissorRects(1, &scissorRect);

    const auto rtvHandle = d3d->GetRtvHandle();

    const CD3DX12_CPU_DESCRIPTOR_HANDLE dsvHandle(d3d->GetDsvHeapStart(), d3d->GetFrameIndex(), d3d->GetDsvDescriptorSize());
    cmdList->OMSetRenderTargets(1, &rtvHandle, FALSE, &dsvHandle);

    cmdList->ClearRenderTargetView(rtvHandle, Config::GetRender().RtvClearColor, 1, &scissorRect);
    cmdList->ClearDepthStencilView(dsvHandle, D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.0f, 0, 0, nullptr);

    CbvMatrices matrices = {};
    XMStoreFloat4x4(&matrices.V, V);
    XMStoreFloat4x4(&matrices.P, P);

    //const CD3DX12_GPU_DESCRIPTOR_HANDLE bindlessHandle(heap->GetGPUHandle(), heap->GetBindlessTexBase(), heap->GetIncrementSize());

    cmdList->SetGraphicsRootSignature(m_rootSig.Get());
    cmdList->SetPipelineState(m_shader.GetPSO());
    cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    D3D12_VERTEX_BUFFER_VIEW viewV;
    viewV.BufferLocation = scene->GPU.MegaBufferVertex.GetResource()->GetGPUVirtualAddress();
    viewV.SizeInBytes = scene->CPU.MegaBufferVertex.size() * sizeof(Vertex);
    viewV.StrideInBytes = sizeof(Vertex);

    D3D12_INDEX_BUFFER_VIEW viewI;
    viewI.BufferLocation = scene->GPU.MegaBufferIndex.GetResource()->GetGPUVirtualAddress();
    viewI.SizeInBytes = scene->CPU.MegaBufferIndex.size() * sizeof(uint32_t);
    viewI.Format = DXGI_FORMAT_R32_UINT;

    cmdList->IASetVertexBuffers(0, 1, &viewV);
    cmdList->IASetIndexBuffer(&viewI);

    {
        GPU_SCOPE(cmdList, "Skybox Pass");

        //if (skybox)
        //    skybox->RenderForward(d3d, cmdList, vMatrix, pMatrix);
    }

    for (int i = 0; i < scene->CPU.Objects.size(); ++i)
    {
        const Object& obj = scene->CPU.Objects[i];
        const InstanceData& instanceData = scene->CPU.MegaBufferInstanceData[i];

        GPU_SCOPE(cmdList, obj.DebugName.c_str());

        matrices.M = instanceData.M;
        matrices.MTI = instanceData.MTI;
        m_descriptorSet.UpdateCBV(0, &matrices);

        m_descriptorSet.TransitionAllSRVToShaderResource(cmdList);
        m_descriptorSet.SetDescriptorTables_Graphics(cmdList);

        //cmdList->SetGraphicsRootDescriptorTable(2, bindlessHandle);

        const size_t byteOffsetVertex = obj.MegaBufferVertexOffset * sizeof(Vertex);
        const size_t byteOffsetIndex = obj.MegaBufferIndexOffset * sizeof(uint32_t);

        cmdList->DrawIndexedInstanced(obj.MegaBufferIndexCount, 1, byteOffsetIndex, byteOffsetVertex, 0);
    }
}
