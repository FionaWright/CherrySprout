#include "System/pch.h"

#include "Render/GizmoManager.h"

#include "Debug/GPUEventScoped.h"
#include "HWI/Heap.h"
#include "HWI/UploadHeap.h"
#include "Scene/Vertex.h"
#include "System/Config.h"
#include "System/FileHelper.h"
#include "System/TextureLoader.h"
#include "Utils/CBVs.h"
#include "Utils/D3DUtils.h"
#include "Utils/Helper.h"

#define UPLOAD_HEAP_SIZE (256 * 100 + 512)

void GizmoManager::AddGizmo(D3D* d3d, Heap* heap, const XMFLOAT3 position, XMFLOAT4 color, const char* texFilepath)
{
    const auto fullPath = FileHelper::GetAssetFullPath(texFilepath);

    if (!m_textureCache.contains(fullPath))
    {
        ScratchImage scratch;
        D12Resource tex = TextureLoader::LoadTexture2DLDR(d3d->GetDevice(), fullPath.c_str(), scratch);

        UploadHeap uploadHeap;
        uploadHeap.Init(d3d->GetDevice(), Align(tex.GetIntermediateSize(), 512));

        const auto cmdListPtr = d3d->GetAvailableCmdList(D3D12_COMMAND_LIST_TYPE_COPY);
        const auto cmdList = cmdListPtr.Get();
        {
            GPU_SCOPE(cmdList, "Upload Gizmo Texture");
            TextureLoader::UploadTexture(cmdList, &uploadHeap, scratch, &tex);
        }
        V(cmdList->Close());
        d3d->ExecuteCommandList(cmdList);
        d3d->Flush();

        m_textureCache[fullPath] = tex;
    }

    if (!m_uploadHeap.IsInitialized())
        m_uploadHeap.Init(d3d->GetDevice(), UPLOAD_HEAP_SIZE);

    Gizmo& gizmo = m_gizmos.emplace_back();
    gizmo.Position = position;
    gizmo.TextureFilepath = fullPath;

    gizmo.DescSet.Init(heap, false, true);
    gizmo.DescSet.AddCBV(d3d->GetDevice(), sizeof(CbvColor), &m_uploadHeap);
    gizmo.DescSet.UpdateCBV(0, &color);
    gizmo.DescSet.SetSRV_Tex2D(d3d->GetDevice(), 0, &m_textureCache[fullPath], m_textureCache[fullPath].GetDesc().Format);
}

void GizmoManager::Render(D3D* d3d, const Heap* heap, ID3D12GraphicsCommandList* cmdList, const XMMATRIX& V, const XMMATRIX& P)
{
    GPU_SCOPE(cmdList, "Gizmos");

    if (!m_initializedResources)
    {
        initResources(d3d, cmdList);
        m_initializedResources = true;
    }

    {
        SetViewportScissor(cmdList, Config::GetSystem().RtvWidth, Config::GetSystem().RtvHeight);
    }

    {
        D3D12_VERTEX_BUFFER_VIEW viewV;
        viewV.BufferLocation = m_quadVertexBuffer.GetResource()->GetGPUVirtualAddress();
        viewV.SizeInBytes = 6 * sizeof(Vertex);
        viewV.StrideInBytes = sizeof(Vertex);

        m_quadVertexBuffer.Transition(cmdList, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);

        cmdList->IASetVertexBuffers(0, 1, &viewV);
        cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

        cmdList->SetGraphicsRootSignature(m_rootSig.Get());
        cmdList->SetPipelineState(m_pipeline.GetPSO());
        heap->Bind(cmdList);
    }

    CbvMatrices_MVP_Lean matrices = {};
    XMStoreFloat4x4(&matrices.V, V);
    XMStoreFloat4x4(&matrices.P, P);

    for (int i = 0; i < m_gizmos.size(); ++i)
    {
        const Gizmo& gizmo = m_gizmos[i];

        gizmo.DescSet.TransitionAllSRVToShaderResource(cmdList);
        gizmo.DescSet.SetDescriptorTables_Graphics(cmdList);

        const auto T = XMMatrixTranslation(gizmo.Position.x, gizmo.Position.y, gizmo.Position.z);
        XMStoreFloat4x4(&matrices.M, T);

        m_rootConstants.Bind_Graphics(cmdList, &matrices);

        cmdList->DrawInstanced(6, 1, 0, 0);
    }
}

void GizmoManager::initResources(D3D* d3d, ID3D12GraphicsCommandList* cmdList)
{
    const Vertex quadVertices[] =
    {
        // Triangle 1
        { { -0.5f, -0.5f, 0.0f }, { 0.0f, 0.0f, 1.0f }, { 0.0f, 1.0f } },
        { { -0.5f,  0.5f, 0.0f }, { 0.0f, 0.0f, 1.0f }, { 0.0f, 0.0f } },
        { {  0.5f,  0.5f, 0.0f }, { 0.0f, 0.0f, 1.0f }, { 1.0f, 0.0f } },

        // Triangle 2
        { { -0.5f, -0.5f, 0.0f }, { 0.0f, 0.0f, 1.0f }, { 0.0f, 1.0f } },
        { {  0.5f,  0.5f, 0.0f }, { 0.0f, 0.0f, 1.0f }, { 1.0f, 0.0f } },
        { {  0.5f, -0.5f, 0.0f }, { 0.0f, 0.0f, 1.0f }, { 1.0f, 1.0f } },
    };

    if (!m_uploadHeap.IsInitialized())
        m_uploadHeap.Init(d3d->GetDevice(), UPLOAD_HEAP_SIZE);

    m_quadVertexBuffer.Init_Buffer("Gizmo Quad Vertex Buffer", d3d->GetDevice(), sizeof(quadVertices));
    m_quadVertexBuffer.UploadBuffer(cmdList, &m_uploadHeap, quadVertices, sizeof(quadVertices));

    D3D12_STATIC_SAMPLER_DESC sampler = {};
    InitializeSamplerLinearClamp(&sampler);

    m_rootConstants.Init(0, 0, sizeof(CbvMatrices_MVP_Lean));
    m_rootSig.SmartInit(d3d->GetDevice(), 1, 1, 0, false, &sampler, 1, &m_rootConstants);

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
    m_pipeline.InitGraphics(d3d->GetDevice(), "Raster/GizmoVSPS.hlsl", "Raster/GizmoVSPS.hlsl", desc);
}
