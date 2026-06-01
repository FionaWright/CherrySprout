//
// Created by fiona on 06/10/2025.
//

#include "System/pch.h"
#include "HWI/DescriptorSet.h"

#include "HWI/Heap.h"
//#include "HWI/TLAS.h"
#include "HWI/UploadHeap.h"
#include "System/Config.h"
#include "Utils/D3DUtils.h"
#include "Utils/Helper.h"

DescriptorSet::~DescriptorSet()
{
    CherryPrint("DescriptorSet Destroyed!");
}

void DescriptorSet::Init(Heap* heap, bool hasBindlessParam)
{
    m_pHeap = heap;

    m_cpuHandle = m_pHeap->GetCPUHandle();
    m_gpuHandle = m_pHeap->GetGPUHandle();
    m_descriptorIncSize = m_pHeap->GetIncrementSize();

    m_hasBindlessParam = hasBindlessParam;
}

void DescriptorSet::AddCBV(ID3D12Device* device, size_t size, UploadHeap* uploadHeap, const char* debugName)
{
    CBV cbv;
    cbv.Size = size;

    // Allocate space in upload heap
    const size_t assignedOffset = uploadHeap->GetAssignedUploadOffset(size, 256);
    cbv.MappedGpuPtr = uploadHeap->GetMappedPointer(assignedOffset);

    // Get descriptor idx / handle
    cbv.HeapIndex = m_pHeap->GetNextDescriptorIdx(debugName);
    const auto handle = m_pHeap->GetDescriptorHandleAtIndex(cbv.HeapIndex);

    // Create CBV
    D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc = {};
    cbvDesc.BufferLocation = uploadHeap->GetUploadResource()->GetGPUVirtualAddress() + assignedOffset;
    cbvDesc.SizeInBytes = static_cast<uint32_t>(Align(size, 256));
    device->CreateConstantBufferView(&cbvDesc, handle);

    // Add to list
    m_cbvs.emplace_back(cbv);
}

void DescriptorSet::UpdateCBV(const uint32_t regIdx, const void* data) const
{
    if (regIdx >= m_cbvs.size())
        throw std::exception("You made a mistake :(");

    const CBV& cbv = m_cbvs[regIdx];
    std::memcpy(cbv.MappedGpuPtr, data, cbv.Size);
}

void DescriptorSet::AddSRV(ID3D12Device* device, D12Resource* d12Resource,
                           const D3D12_SHADER_RESOURCE_VIEW_DESC& desc, const char* debugName)
{
    SRV srv;
    srv.HeapIndex = m_pHeap->GetNextDescriptorIdx(debugName);

    if (d12Resource)
    {
        const auto handle = m_pHeap->GetDescriptorHandleAtIndex(srv.HeapIndex);
        device->CreateShaderResourceView(d12Resource->GetResource(), &desc, handle);
    }

    srv.D12Resource = d12Resource;
    m_srvs.emplace_back(srv);
}

void DescriptorSet::SetSRV(ID3D12Device* device, const uint32_t srvIdx, D12Resource* d12Resource,
                           const D3D12_SHADER_RESOURCE_VIEW_DESC& desc, const char* debugName)
{
    if (srvIdx > m_srvs.size())
        throw std::exception("Invalid srv index");

    ID3D12Resource* resource = d12Resource ? d12Resource->GetResource() : nullptr;

    if (srvIdx == m_srvs.size())
    {
        AddSRV(device, d12Resource, desc, debugName);
        return;
    }

    SRV& srv = m_srvs.at(srvIdx);
    srv.D12Resource = d12Resource;
    const auto handle = m_pHeap->GetDescriptorHandleAtIndex(srv.HeapIndex);
    device->CreateShaderResourceView(resource, &desc, handle);
}

void DescriptorSet::SetSRV_Tex2D(ID3D12Device* device, const uint32_t srvIdx, D12Resource* d12Resource)
{
    D3D12_SHADER_RESOURCE_VIEW_DESC desc = {};
    desc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
    desc.Format = d12Resource->GetDesc().Format;
    desc.Texture2D.MipLevels = d12Resource->GetDesc().MipLevels;
    desc.Texture2D.MostDetailedMip = 0;
    desc.Texture2D.PlaneSlice = 0;
    desc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;

    const char* debugName = Config::GetSystem().DebugHeapEnabled ? d12Resource->GetName().c_str() : nullptr;
    SetSRV(device, srvIdx, d12Resource, desc, debugName);
}

void DescriptorSet::SetSRV_Buffer(ID3D12Device* device, const uint32_t srvIdx, D12Resource* d12Resource,
                                  const uint32_t numElements, const size_t stride)
{
    D3D12_SHADER_RESOURCE_VIEW_DESC desc = {};
    desc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
    desc.Buffer.FirstElement = 0;
    desc.Buffer.NumElements = numElements;
    desc.Buffer.StructureByteStride = stride;
    desc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;

    const char* debugName = Config::GetSystem().DebugHeapEnabled ? d12Resource->GetName().c_str() : nullptr;
    SetSRV(device, srvIdx, d12Resource, desc, debugName);
}

void DescriptorSet::SetSRV_RTAS(ID3D12Device* device, const uint32_t srvIdx, const D12Resource* d12Resource)
{
    D3D12_SHADER_RESOURCE_VIEW_DESC desc = {};
    desc.ViewDimension = D3D12_SRV_DIMENSION_RAYTRACING_ACCELERATION_STRUCTURE;
    if (d12Resource)
        desc.RaytracingAccelerationStructure.Location = d12Resource->GetResource()->GetGPUVirtualAddress();
    desc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;

    const char* debugName = Config::GetSystem().DebugHeapEnabled && d12Resource ? d12Resource->GetName().c_str() : nullptr;
    SetSRV(device, srvIdx, nullptr, desc, debugName);
}

void DescriptorSet::TransitionAllSRVToShaderResource(ID3D12GraphicsCommandList* cmdList) const
{
    for (int i = 0; i < m_srvs.size(); i++)
    {
        if (m_srvs[i].D12Resource)
        {
            m_srvs[i].D12Resource->Transition(cmdList, D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE);
        }
    }
}

void DescriptorSet::AddUAV(ID3D12Device* device, ID3D12Resource* resource, const D3D12_UNORDERED_ACCESS_VIEW_DESC& desc)
{
    UAV uav;

    uav.HeapIndex = m_pHeap->GetNextDescriptorIdx("UAV");

    if (resource)
    {
        const auto handle = m_pHeap->GetDescriptorHandleAtIndex(uav.HeapIndex);
        device->CreateUnorderedAccessView(resource, nullptr, &desc, handle);
    }

    m_uavs.emplace_back(uav);
}

void DescriptorSet::SetUAV(ID3D12Device* device, const uint32_t uavIdx, ID3D12Resource* resource,
                           const D3D12_UNORDERED_ACCESS_VIEW_DESC& desc)
{
    if (uavIdx > m_uavs.size())
        throw std::exception("Invalid uav index");

    if (uavIdx == m_uavs.size())
    {
        AddUAV(device, resource, desc);
        return;
    }

    if (resource)
    {
        const UAV& uav = m_uavs.at(uavIdx);

        const auto handle = m_pHeap->GetDescriptorHandleAtIndex(uav.HeapIndex);
        device->CreateUnorderedAccessView(resource, nullptr, &desc, handle);
    }
}

void DescriptorSet::SetUAV_Tex2D(ID3D12Device* device, const uint32_t uavIdx, const D12Resource* d12Resource)
{
    D3D12_UNORDERED_ACCESS_VIEW_DESC desc = {};
    desc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
    desc.Format = d12Resource->GetDesc().Format;
    desc.Texture2D.MipSlice = 0;
    desc.Texture2D.PlaneSlice = 0;

    SetUAV(device, uavIdx, d12Resource->GetResource(), desc);
}

void DescriptorSet::SetDescriptorTables_Graphics(ID3D12GraphicsCommandList* cmdList) const
{
    setDescriptorTables(cmdList, false);
}

void DescriptorSet::SetDescriptorTables_Compute(ID3D12GraphicsCommandList* cmdList) const
{
    setDescriptorTables(cmdList, true);
}

void DescriptorSet::setDescriptorTables(ID3D12GraphicsCommandList* cmdList, const bool isCompute) const
{
    int paramIdx = 0;

    if (m_cbvs.size() > 0)
    {
        const CD3DX12_GPU_DESCRIPTOR_HANDLE cbvHandle(m_gpuHandle, m_cbvs[0].HeapIndex,
                                                      m_descriptorIncSize);
        if (isCompute)
            cmdList->SetComputeRootDescriptorTable(paramIdx, cbvHandle);
        else
            cmdList->SetGraphicsRootDescriptorTable(paramIdx, cbvHandle);
        paramIdx++;
    }

    if (m_srvs.size() > 0)
    {
        const CD3DX12_GPU_DESCRIPTOR_HANDLE srvHandle(m_gpuHandle, m_srvs[0].HeapIndex,
                                                      m_descriptorIncSize);
        if (isCompute)
            cmdList->SetComputeRootDescriptorTable(paramIdx, srvHandle);
        else
            cmdList->SetGraphicsRootDescriptorTable(paramIdx, srvHandle);
        paramIdx++;
    }

    if (m_hasBindlessParam)
        paramIdx++;

    if (m_uavs.size() > 0)
    {
        const CD3DX12_GPU_DESCRIPTOR_HANDLE uavHandle(m_gpuHandle, m_uavs[0].HeapIndex,
                                                      m_descriptorIncSize);
        if (isCompute)
            cmdList->SetComputeRootDescriptorTable(paramIdx, uavHandle);
        else
            cmdList->SetGraphicsRootDescriptorTable(paramIdx, uavHandle);
        paramIdx++;
    }
}
