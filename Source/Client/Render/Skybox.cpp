//
// Created by fiona on 14/01/2026.
//

#include "System/pch.h"
#include "Render/Skybox.h"
#include "Debug/GPUEventScoped.h"
#include "HWI/Heap.h"
#include "Utils/CommonStructs.h"
#include "Utils/D3DUtils.h"

void Skybox::Init(ID3D12Device* device, ID3D12GraphicsCommandList* cmdList, Heap* heap, UploadHeap* uploadHeap, D12Resource* cubemap)
{
    D3D12_STATIC_SAMPLER_DESC sampler;
    InitializeSamplerLinearClamp(&sampler);

    m_rootSig.SmartInit(device, 1, 1, 0, false, &sampler, 1);

    D3D12_INPUT_ELEMENT_DESC rasterILD[] =
    {
        {
            "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT,
            D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0
        }
    };
    m_shaderForward.InitVsPs("Raster/SkyboxVS.hlsl", "Raster/SkyboxPS.hlsl", {rasterILD, _countof(rasterILD)}, device, m_rootSig.Get(), true);

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

    m_cubeVertexBuffer.Init_Buffer("Cube Vertex Buffer", device, sizeof(XMFLOAT3) * 8);
    m_cubeVertexBuffer.UploadBuffer(cmdList, uploadHeap, vertexBuffer, sizeof(XMFLOAT3) * 8);

    m_cubeIndexBuffer.Init_Buffer("Cube Index Buffer", device, sizeof(uint32_t) * 36);
    m_cubeIndexBuffer.UploadBuffer(cmdList, uploadHeap, indexBuffer, sizeof(uint32_t) * 36);

    // Init Generate Irradiance
    {
        m_rootSigGenIrr.SmartInit(device, 0, 1, 1, false, &sampler, 1);
        m_shaderGenIrr.InitCs("Compute/GenIrradianceIblCS.hlsl", device, m_rootSigGenIrr.Get());

        if (!m_texIrradianceIBL.IsInitialized())
            m_texIrradianceIBL.Init_Tex2D("Irradiance IBL", device, cubemap->GetDesc().Width, cubemap->GetDesc().Height, 6, DXGI_FORMAT_R8G8B8A8_UNORM, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);
    }
}

void Skybox::RenderForward(const D3D* d3d, ID3D12GraphicsCommandList* cmdList, Heap* heap, const XMMATRIX& vMatrix, const XMMATRIX& pMatrix) const
{
    CbvMatrices matrices = {};
    XMStoreFloat4x4(&matrices.V, vMatrix);
    XMStoreFloat4x4(&matrices.P, pMatrix);

    cmdList->SetGraphicsRootSignature(m_rootSig.Get());
    cmdList->SetPipelineState(m_shaderForward.GetPSO());

    m_dsForwardRender.UpdateCBV(0, &matrices);

    m_dsForwardRender.TransitionAllSRVToShaderResource(cmdList);
    m_dsForwardRender.SetDescriptorTables_Graphics(cmdList);

    D3D12_VERTEX_BUFFER_VIEW viewV;
    D3D12_INDEX_BUFFER_VIEW viewI;
    VertexIndexBuffersToViews(&m_cubeVertexBuffer, &m_cubeIndexBuffer, 8, sizeof(XMFLOAT3), 36, viewV, viewI);

    cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    cmdList->IASetVertexBuffers(0, 1, &viewV);
    cmdList->IASetIndexBuffer(&viewI);
    cmdList->DrawIndexedInstanced(36, 1, 0, 0, 0);
}

void Skybox::UpdateCubemap(ID3D12Device* device, D12Resource* cubemap, Heap* heap, UploadHeap* uploadHeap)
{
    D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
    srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURECUBE;
    srvDesc.Format = cubemap->GetDesc().Format;
    srvDesc.TextureCube.MipLevels = 1;
    srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;

    // Forward Render Material
    {
        m_dsForwardRender.Init(heap);
        m_dsForwardRender.AddCBV(device, sizeof(CbvMatrices), uploadHeap, "CBV Matrices (Skybox)");
        m_dsForwardRender.SetSRV(device, 0, cubemap, srvDesc);
    }

    // Generate Irradiance Material
    {
        D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
        uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2DARRAY;
        uavDesc.Format = m_texIrradianceIBL.GetDesc().Format;
        uavDesc.Texture2DArray.ArraySize = 6;
        uavDesc.Texture2DArray.MipSlice = 0;
        uavDesc.Texture2DArray.FirstArraySlice = 0;

        m_dsGenIrr.Init(heap);
        m_dsGenIrr.AddUAV(device, m_texIrradianceIBL.GetResource(), uavDesc);
        m_dsGenIrr.SetSRV_Tex2D(device, 0, cubemap);
    }
}

void Skybox::GenerateIrradianceMap(ID3D12GraphicsCommandList* cmdList, const Heap* heap)
{
    GPU_SCOPE(cmdList, "Generate Irradiance Map");

    m_texIrradianceIBL.Transition(cmdList, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
    m_dsGenIrr.TransitionAllSRVToShaderResource(cmdList);

    heap->Bind(cmdList);
    cmdList->SetComputeRootSignature(m_rootSigGenIrr.Get());
    cmdList->SetPipelineState(m_shaderGenIrr.GetPSO());
    m_dsGenIrr.SetDescriptorTables_Compute(cmdList);

    const uint32_t groupWidth = (m_texIrradianceIBL.GetDesc().Width + 7) / 8;
    const uint32_t groupHeight = (m_texIrradianceIBL.GetDesc().Height + 7) / 8;

    cmdList->Dispatch(groupWidth, groupHeight, 1);
}
