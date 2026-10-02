//
// Created by fiona on 14/01/2026.
//

#include "System/pch.h"
#include "Render/Skybox.h"
#include "Debug/GPUEventScoped.h"
#include "HWI/Heap.h"
#include "HWI/UploadHeap.h"
#include "Utils/CBVs.h"
#include "Utils/D3DUtils.h"
#include "Utils/Helper.h"

void Skybox::Init(D3D* d3d, D12Resource* cubemap)
{
    D3D12_STATIC_SAMPLER_DESC sampler;
    InitializeSamplerLinearClamp(&sampler);

    m_rootSig.SmartInit(d3d->GetDevice(), 1, 1, 0, false, &sampler, 1);

    D3D12_INPUT_ELEMENT_DESC rasterILD[] =
    {
        {
            "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT,
            D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0
        }
    };

    // TODO: Fix this
    auto desc = CreateGraphicsPipelineDesc(m_rootSig.Get(), { rasterILD, _countof(rasterILD) }, true);
    m_pipelineForward.InitGraphics(d3d->GetDevice(), "Raster/SkyboxVS.hlsl", "Raster/SkyboxManualPS.hlsl", desc);

    constexpr XMFLOAT3 vertexBuffer[8] = {
        {-1, -1, -1}, {1, -1, -1}, {1, 1, -1}, {-1, 1, -1},
        {-1, -1,  1}, {1, -1,  1}, {1, 1,  1}, {-1, 1,  1}
    };

    constexpr uint32_t indexBuffer[36] = {
        0, 1, 2,  // back face
        0, 2, 3,
        4, 6, 5,  // front face
        4, 7, 6,
        0, 3, 7,  // left face
        0, 7, 4,
        1, 6, 2,  // right face
        1, 5, 6,
        0, 5, 1,  // bottom face
        0, 4, 5,
        3, 6, 7,  // top face
        3, 2, 6
    };

    m_cubeVertexBuffer.Init_Buffer("Cube Vertex Buffer", d3d->GetDevice(), sizeof(XMFLOAT3) * 8);
    m_cubeIndexBuffer.Init_Buffer("Cube Index Buffer", d3d->GetDevice(), sizeof(uint32_t) * 36);

    {
        const auto cmdListPtr = d3d->GetAvailableCmdList(D3D12_COMMAND_LIST_TYPE_COPY);
        const auto cmdList = cmdListPtr.Get();

        const size_t uploadHeapRequiredSize = m_cubeVertexBuffer.GetIntermediateSize() + m_cubeIndexBuffer.GetIntermediateSize();

        UploadHeap uploadHeap;
        uploadHeap.Init(d3d->GetDevice(), uploadHeapRequiredSize);

        m_cubeVertexBuffer.UploadBuffer(cmdList, &uploadHeap, vertexBuffer, sizeof(XMFLOAT3) * 8);
        m_cubeIndexBuffer.UploadBuffer(cmdList, &uploadHeap, indexBuffer, sizeof(uint32_t) * 36);

        V(cmdList->Close());
        d3d->ExecuteCommandList(cmdList);
        d3d->Flush();
    }

    // Init Generate Irradiance
    {
        m_rootSigGenIrr.SmartInit(d3d->GetDevice(), 0, 1, 1, false, &sampler, 1);
        m_pipelineGenIrr.InitCompute(d3d->GetDevice(), "Compute/GenIrradianceIblCS.hlsl", m_rootSigGenIrr.Get());

        if (!m_texIrradianceIBL.IsInitialized())
            m_texIrradianceIBL.Init_Tex2D("Irradiance IBL", d3d->GetDevice(), cubemap->GetDesc().Width, cubemap->GetDesc().Height, 6, DXGI_FORMAT_R8G8B8A8_UNORM, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);
    }
}

void Skybox::RenderForward(ID3D12GraphicsCommandList* cmdList, const XMMATRIX* vMatrix, const XMMATRIX* pMatrix) const
{
    CbvMatrices_MVP matrices = {};
    XMStoreFloat4x4(&matrices.V, *vMatrix);
    XMStoreFloat4x4(&matrices.P, *pMatrix);

    cmdList->SetGraphicsRootSignature(m_rootSig.Get());
    cmdList->SetPipelineState(m_pipelineForward.GetPSO());

    m_setForwardRender.UpdateCBV(0, &matrices);

    m_setForwardRender.TransitionAllSRVToShaderResource(cmdList);
    m_setForwardRender.SetDescriptorTables_Graphics(cmdList);

    D3D12_VERTEX_BUFFER_VIEW viewV;
    D3D12_INDEX_BUFFER_VIEW viewI;
    VertexIndexBuffersToViews(&m_cubeVertexBuffer, &m_cubeIndexBuffer, 8, sizeof(XMFLOAT3), 36, viewV, viewI);

    cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    cmdList->IASetVertexBuffers(0, 1, &viewV);
    cmdList->IASetIndexBuffer(&viewI);
    cmdList->DrawIndexedInstanced(36, 1, 0, 0, 0);
}

void Skybox::UpdateDescriptorSet(ID3D12Device* device, D12Resource* cubemap, Heap* heap, UploadHeap* uploadHeapCBV)
{
    D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
    //srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURECUBE;
    srvDesc.Format = cubemap->GetDesc().Format;
    srvDesc.TextureCube.MipLevels = 1;
    srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;

    // TODO: Fix this
    srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2DARRAY;
    srvDesc.Texture2DArray.ArraySize = 6;
    srvDesc.Texture2DArray.MipLevels = 1;
    srvDesc.Texture2DArray.FirstArraySlice = 0;

    {
        m_setForwardRender.Init(heap);
        m_setForwardRender.AddCBV(device, sizeof(CbvMatrices_MVP), uploadHeapCBV, "CBV Matrices (Skybox)");
        m_setForwardRender.SetSRV(device, 0, cubemap, srvDesc);
    }

    {
        D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
        uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2DARRAY;
        uavDesc.Format = m_texIrradianceIBL.GetDesc().Format;
        uavDesc.Texture2DArray.ArraySize = 6;
        uavDesc.Texture2DArray.MipSlice = 0;
        uavDesc.Texture2DArray.FirstArraySlice = 0;

        m_setGenIrr.Init(heap);
        m_setGenIrr.AddUAV(device, &m_texIrradianceIBL, uavDesc);
        m_setGenIrr.SetSRV(device, 0, cubemap, srvDesc);
    }
}

void Skybox::GenerateIrradianceMap(D3D* d3d, const Heap* heap)
{
    const auto cmdListPtr = d3d->GetAvailableCmdList(D3D12_COMMAND_LIST_TYPE_DIRECT);
    const auto cmdList = cmdListPtr.Get();
    {
        GPU_SCOPE(cmdList, "Generate Irradiance Map");

        m_texIrradianceIBL.Transition(cmdList, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
        m_setGenIrr.TransitionAllSRVToShaderResource(cmdList);

        heap->Bind(cmdList);
        cmdList->SetComputeRootSignature(m_rootSigGenIrr.Get());
        cmdList->SetPipelineState(m_pipelineGenIrr.GetPSO());
        m_setGenIrr.SetDescriptorTables_Compute(cmdList);

        DispatchOverTexture(cmdList, 8, m_texIrradianceIBL.GetDesc().Width, m_texIrradianceIBL.GetDesc().Height, 1);
    }
    V(cmdList->Close());
    d3d->ExecuteCommandList(cmdList);
    d3d->Flush();
}
