//
// Created by fionaw on 01/06/2026.
//

#include "System/pch.h"
#include "HWI/RtasBuilder.h"

#include "HWI/D3D.h"
#include "Scene/InstanceData.h"
#include "Utils/Helper.h"

void RtasBuilder::FlushAndBuild(D3D* d3d, Scene* scene)
{
    // Note: Direct queue is required as you can't transition from D3D12_RESOURCE_STATE_INDEX_BUFFER in a compute queue. Buffer may be left in that state from previous rasterization passes
    d3d->Flush();
    const auto cmdListPtr = d3d->GetAvailableCmdList(D3D12_COMMAND_LIST_TYPE_DIRECT);
    const auto cmdList = cmdListPtr.Get();
    {
        ComPtr<ID3D12Device5> device5;
        V(d3d->GetDevice()->QueryInterface(IID_PPV_ARGS(&device5)));
        ComPtr<ID3D12GraphicsCommandList4> cmdList4;
        V(cmdList->QueryInterface(IID_PPV_ARGS(&cmdList4)));

        Build(device5.Get(), cmdList4.Get(), scene);
    }
    V(cmdList->Close());
    d3d->ExecuteCommandList(cmdList);
    d3d->Flush();
}

void RtasBuilder::Build(ID3D12Device5* device, ID3D12GraphicsCommandList4* cmdList, Scene* scene)
{
    CherryPrint("Building RTAS...");

    scene->GPU.MegaBufferVertex.Transition(cmdList, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
    scene->GPU.MegaBufferIndex.Transition(cmdList, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);

    m_blasList.clear();
    m_megaBufferInstanceData.clear();

    m_uploadHeap = {};
    const size_t instanceSizeUser = sizeof(InstanceData) * scene->CPU.ObjectCount;
    const size_t instanceSizeInternal = sizeof(D3D12_RAYTRACING_INSTANCE_DESC) * scene->CPU.ObjectCount;
    m_uploadHeap.Init(device, 64 + instanceSizeUser + instanceSizeInternal);

    for (int i = 0; i < scene->CPU.ObjectCount; i++)
    {
        buildBlas(device, cmdList, scene->GPU.MegaBufferVertex.GetResource(), scene->GPU.MegaBufferIndex.GetResource(), &scene->CPU.Objects[i]);
    }

    std::vector<D3D12_RAYTRACING_INSTANCE_DESC> blasInstances;
    for (int i = 0; i < m_blasList.size(); i++)
    {
        const Object& obj = scene->CPU.Objects[i];
        const D12Resource& blasResult = m_blasList[i].Result;

        const XMMATRIX M = XMMatrixSet(
            obj.M[0], obj.M[1], obj.M[2], obj.M[3],
            obj.M[4], obj.M[5], obj.M[6], obj.M[7],
            obj.M[8], obj.M[9], obj.M[10], obj.M[11],
            obj.M[12], obj.M[13], obj.M[14], obj.M[15]
        );

        const XMMATRIX MTI = XMMatrixTranspose(XMMatrixInverse(nullptr, M));

        XMFLOAT3X4 M3x4{};
        XMStoreFloat3x4(&M3x4, M);

        D3D12_RAYTRACING_INSTANCE_DESC blasInstance = {};
        for (int c = 0; c < 3; c++)
            for (int r = 0; r < 4; r++)
                blasInstance.Transform[c][r] = M3x4.m[c][r];
        blasInstance.InstanceID = i;
        blasInstance.InstanceContributionToHitGroupIndex = 0;
        blasInstance.InstanceMask = 0xFF;
        blasInstance.AccelerationStructure = blasResult.GetResource()->GetGPUVirtualAddress();
        blasInstance.Flags = D3D12_RAYTRACING_INSTANCE_FLAG_NONE;
        blasInstances.emplace_back(blasInstance);

        InstanceData instanceData{};
        XMStoreFloat4x4(&instanceData.M, M);
        XMStoreFloat4x4(&instanceData.MTI, MTI);
        instanceData.MegaBufferOffsetVertex = obj.MegaBufferVertexOffset;
        instanceData.MegaBufferOffsetIndex = obj.MegaBufferIndexOffset;
        instanceData.MegaBufferCountIndex = obj.MegaBufferIndexCount;
        instanceData.MaterialIndex = obj.MaterialIndex;
        m_megaBufferInstanceData.emplace_back(instanceData);
    }

    scene->GPU.MegaBufferInstanceData.Release();
    scene->GPU.MegaBufferInstanceData.Init_Buffer("Mega Buffer Instance Data", device, instanceSizeUser);
    scene->GPU.MegaBufferInstanceData.UploadBuffer(cmdList, &m_uploadHeap, m_megaBufferInstanceData.data(), instanceSizeUser);

    buildTlas(device, cmdList, blasInstances);

    CherryPrint("RTAS Built.");
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
    CherryAssert(vertexByteOffset < vertexBuffer->GetDesc().Width);
    CherryAssert(indexByteOffset < indexBuffer->GetDesc().Width);

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

    CherryAssert(prebuildInfo.ScratchDataSizeInBytes > 0);
    CherryAssert(prebuildInfo.ResultDataMaxSizeInBytes > 0);

    entry.Scratch.Init_Buffer("BLAS Scratch", device, prebuildInfo.ScratchDataSizeInBytes, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);
    entry.Result.Init_Buffer("BLAS Result", device, prebuildInfo.ResultDataMaxSizeInBytes, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS, false, D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE);

    D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC buildDesc = {};
    buildDesc.Inputs = inputs;
    buildDesc.ScratchAccelerationStructureData = entry.Scratch.GetResource()->GetGPUVirtualAddress();
    buildDesc.DestAccelerationStructureData = entry.Result.GetResource()->GetGPUVirtualAddress();

    cmdList->BuildRaytracingAccelerationStructure(&buildDesc, 0, nullptr);

    D3D12_RESOURCE_BARRIER barrier = {};
    barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_UAV;
    barrier.UAV.pResource = nullptr;
    cmdList->ResourceBarrier(1, &barrier);

    m_blasList.emplace_back(std::move(entry));
}

void RtasBuilder::buildTlas(ID3D12Device5* device, ID3D12GraphicsCommandList4* cmdList, const std::vector<D3D12_RAYTRACING_INSTANCE_DESC>& blasInstances)
{
    const UINT64 bufferSize = sizeof(D3D12_RAYTRACING_INSTANCE_DESC) * blasInstances.size();

    m_tlasInstanceBuffer.Release();
    m_tlasInstanceBuffer.Init_Buffer("TLAS Instance Upload Buffer", device, bufferSize);
    m_tlasInstanceBuffer.UploadBuffer(cmdList, &m_uploadHeap, blasInstances.data(), bufferSize);
    m_tlasInstanceBuffer.Transition(cmdList, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);

    D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS inputs = {};
    inputs.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL;
    inputs.Flags = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PREFER_FAST_TRACE;
    inputs.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY;
    inputs.NumDescs = static_cast<UINT>(blasInstances.size());
    inputs.InstanceDescs = m_tlasInstanceBuffer.GetResource()->GetGPUVirtualAddress();

    // Compute GPU memory size needed
    D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO prebuildInfo = {};
    device->GetRaytracingAccelerationStructurePrebuildInfo(&inputs, &prebuildInfo);

    m_tlasScratch.Release();
    m_tlasScratch.Init_Buffer("TLAS Scratch", device, prebuildInfo.ScratchDataSizeInBytes, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);

    m_tlasResult.Release();
    m_tlasResult.Init_Buffer("TLAS Result", device, prebuildInfo.ResultDataMaxSizeInBytes, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS, false, D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE);

    D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC buildDesc = {};
    buildDesc.Inputs = inputs;
    buildDesc.ScratchAccelerationStructureData = m_tlasScratch.GetResource()->GetGPUVirtualAddress();
    buildDesc.DestAccelerationStructureData = m_tlasResult.GetResource()->GetGPUVirtualAddress();

    cmdList->BuildRaytracingAccelerationStructure(&buildDesc, 0, nullptr);

    D3D12_RESOURCE_BARRIER barrier = {};
    barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_UAV;
    barrier.UAV.pResource = nullptr;
    cmdList->ResourceBarrier(1, &barrier);
}
