//
// Created by fionaw on 01/06/2026.
//

#include "System/pch.h"
#include "HWI/RtasBuilder.h"

#include "Scene/InstanceData.h"
#include "Utils/Helper.h"

void RtasBuilder::Build(ID3D12Device5* device, ID3D12GraphicsCommandList4* cmdList, Scene* scene)
{
    std::cout << "Building RTAS..." << std::endl;

    scene->GPU.MegaBufferVertex.Transition(cmdList, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
    scene->GPU.MegaBufferIndex.Transition(cmdList, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);

    m_blasList.clear();
    m_megaBufferInstanceData.clear();

    m_uploadHeap = {};
    m_uploadHeap.Init(device, 64 + scene->CPU.ObjectCount * sizeof(InstanceData));

    m_tlasScratch = {};
    m_tlasResult = {};

    for (int i = 0; i < scene->CPU.ObjectCount; i++)
    {
        buildBlas(device, cmdList, scene->GPU.MegaBufferVertex.GetResource(), scene->GPU.MegaBufferIndex.GetResource(), &scene->CPU.Objects[i]);
    }

    std::vector<D3D12_RAYTRACING_INSTANCE_DESC> blasInstances;
    for (int i = 0; i < m_blasList.size(); i++)
    {
        const Object& obj = scene->CPU.Objects[i];
        const D12Resource& blasResult = m_blasList[i].Result;

        XMMATRIX M = XMMatrixSet(
            obj.M[0], obj.M[1], obj.M[2], obj.M[3],
            obj.M[4], obj.M[5], obj.M[6], obj.M[7],
            obj.M[8], obj.M[9], obj.M[10], obj.M[11],
            obj.M[12], obj.M[13], obj.M[14], obj.M[15]
            );

        //M = XMMatrixTranspose(M);

        const XMMATRIX MTI = XMMatrixTranspose(XMMatrixInverse(nullptr, M));

        D3D12_RAYTRACING_INSTANCE_DESC blasInstance = {};
        XMStoreFloat3x4(reinterpret_cast<XMFLOAT3X4*>(&blasInstance.Transform), M);
        blasInstance.InstanceID = i;
        blasInstance.InstanceContributionToHitGroupIndex = 0;
        blasInstance.InstanceMask = 0xFF;
        blasInstance.AccelerationStructure = blasResult.GetResource()->GetGPUVirtualAddress();
        blasInstance.Flags = D3D12_RAYTRACING_INSTANCE_FLAG_NONE;
        blasInstances.emplace_back(blasInstance);

        InstanceData instanceData;
        XMStoreFloat4x4(&instanceData.M, M);
        XMStoreFloat4x4(&instanceData.MTI, MTI);
        instanceData.MegaBufferOffsetVertex = obj.MegaBufferVertexOffset;
        instanceData.MegaBufferOffsetIndex = obj.MegaBufferIndexOffset;
        instanceData.MaterialIndex = obj.MaterialIndex;
        m_megaBufferInstanceData.emplace_back(instanceData);
    }

    const size_t megaBufferInstanceDataSize = scene->CPU.ObjectCount * sizeof(InstanceData);
    scene->GPU.MegaBufferInstanceData = {};
    scene->GPU.MegaBufferInstanceData.Init_Buffer("Mega Buffer Instance Data", device, megaBufferInstanceDataSize);
    scene->GPU.MegaBufferInstanceData.UploadBuffer(cmdList, &m_uploadHeap, m_megaBufferInstanceData.data(), megaBufferInstanceDataSize);

    buildTlas(device, cmdList, blasInstances);

    std::cout << "RTAS Built." << std::endl;
}

void RtasBuilder::buildBlas(ID3D12Device5* device, ID3D12GraphicsCommandList4* cmdList, ID3D12Resource* vertexBuffer, ID3D12Resource* indexBuffer, Object* object)
{
    D3D12_RAYTRACING_GEOMETRY_DESC geomDesc = {};
    geomDesc.Type = D3D12_RAYTRACING_GEOMETRY_TYPE_TRIANGLES;
    geomDesc.Flags = D3D12_RAYTRACING_GEOMETRY_FLAG_OPAQUE;

    const size_t vertexByteOffset = object->MegaBufferVertexOffset * sizeof(Vertex);
    const size_t indexByteOffset = object->MegaBufferIndexOffset * sizeof(uint32_t);

    CherryAssert(vertexBuffer);
    CherryAssert(vertexBuffer->GetGPUVirtualAddress());

    geomDesc.Triangles.VertexBuffer.StartAddress = vertexBuffer->GetGPUVirtualAddress() + vertexByteOffset;
    geomDesc.Triangles.VertexBuffer.StrideInBytes = sizeof(Vertex);
    geomDesc.Triangles.VertexCount = object->MegaBufferVertexCount;
    geomDesc.Triangles.VertexFormat = DXGI_FORMAT_R32G32B32_FLOAT;

    geomDesc.Triangles.IndexBuffer = indexBuffer->GetGPUVirtualAddress() + indexByteOffset;
    geomDesc.Triangles.IndexFormat = DXGI_FORMAT_R32_UINT;
    geomDesc.Triangles.IndexCount = object->MegaBufferIndexCount;

    D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS inputs = {};
    inputs.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL;
    inputs.Flags = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PREFER_FAST_TRACE;
    inputs.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY;
    inputs.NumDescs = 1;
    inputs.pGeometryDescs = &geomDesc;

    // Compute GPU memory size needed
    D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO prebuildInfo = {};
    device->GetRaytracingAccelerationStructurePrebuildInfo(&inputs, &prebuildInfo);

    BlasEntry entry;

    entry.Scratch.Init_Buffer("BLAS Scratch", device, prebuildInfo.ScratchDataSizeInBytes, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);
    entry.Result.Init_Buffer("BLAS Result", device, prebuildInfo.ResultDataMaxSizeInBytes, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS, false, D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE);

    D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC buildDesc = {};
    buildDesc.Inputs = inputs;
    buildDesc.ScratchAccelerationStructureData = entry.Scratch.GetResource()->GetGPUVirtualAddress();
    buildDesc.DestAccelerationStructureData = entry.Result.GetResource()->GetGPUVirtualAddress();

    cmdList->BuildRaytracingAccelerationStructure(&buildDesc, 0, nullptr);

    D3D12_RESOURCE_BARRIER barrier = {};
    barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_UAV;
    barrier.UAV.pResource = entry.Result.GetResource();
    cmdList->ResourceBarrier(1, &barrier);

    m_blasList.emplace_back(std::move(entry));
}

void RtasBuilder::buildTlas(ID3D12Device5* device, ID3D12GraphicsCommandList4* cmdList, const std::vector<D3D12_RAYTRACING_INSTANCE_DESC>& blasInstances)
{
    const UINT64 bufferSize = sizeof(D3D12_RAYTRACING_INSTANCE_DESC) * blasInstances.size();
    const CD3DX12_HEAP_PROPERTIES uploadHeapProps(D3D12_HEAP_TYPE_UPLOAD);
    const CD3DX12_RESOURCE_DESC bufferDesc = CD3DX12_RESOURCE_DESC::Buffer(bufferSize);

    V(device->CreateCommittedResource(
        &uploadHeapProps,
        D3D12_HEAP_FLAG_NONE,
        &bufferDesc,
        D3D12_RESOURCE_STATE_GENERIC_READ,
        nullptr,
        IID_PPV_ARGS(&m_tlasInstanceBuffer)));
    V(m_tlasInstanceBuffer->SetName(L"TLAS Instance Upload Buffer"));

    void* mappedData = nullptr;
    V(m_tlasInstanceBuffer->Map(0, nullptr, &mappedData));
    memcpy(mappedData, blasInstances.data(), bufferSize);
    m_tlasInstanceBuffer->Unmap(0, nullptr);

    D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS inputs = {};
    inputs.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL;
    inputs.Flags = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PREFER_FAST_TRACE;
    inputs.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY;
    inputs.NumDescs = static_cast<UINT>(blasInstances.size());
    inputs.InstanceDescs = m_tlasInstanceBuffer->GetGPUVirtualAddress();

    // Compute GPU memory size needed
    D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO prebuildInfo = {};
    device->GetRaytracingAccelerationStructurePrebuildInfo(&inputs, &prebuildInfo);

    m_tlasScratch.Init_Buffer("TLAS Scratch", device, prebuildInfo.ScratchDataSizeInBytes, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);
    m_tlasResult.Init_Buffer("TLAS Result", device, prebuildInfo.ResultDataMaxSizeInBytes, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS, false, D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE);

    D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC buildDesc = {};
    buildDesc.Inputs = inputs;
    buildDesc.ScratchAccelerationStructureData = m_tlasScratch.GetResource()->GetGPUVirtualAddress();
    buildDesc.DestAccelerationStructureData = m_tlasResult.GetResource()->GetGPUVirtualAddress();

    cmdList->BuildRaytracingAccelerationStructure(&buildDesc, 0, nullptr);

    D3D12_RESOURCE_BARRIER barrier = {};
    barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_UAV;
    barrier.UAV.pResource = m_tlasResult.GetResource();
    cmdList->ResourceBarrier(1, &barrier);
}
