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

void DescriptorSet::Init(Heap* heap, const bool hasBindlessParam, const bool hasRootConstantsParam)
{
    m_pHeap = heap;

    m_cpuHandle = m_pHeap->GetCPUHandle();
    m_gpuHandle = m_pHeap->GetGPUHandle();
    m_descriptorIncSize = m_pHeap->GetIncrementSize();

    m_hasBindlessParam = hasBindlessParam;
    m_hasRootConstantsParam = hasRootConstantsParam;

    m_cbvs.clear();
    m_srvs.clear();
    m_uavs.clear();
}

void DescriptorSet::AddCBV(ID3D12Device* device, const uint32_t spaceIdx, const size_t size, UploadHeap* uploadHeap, const char* debugName)
{
    CherryAssert(size <= 65536); // 64KB

    CBV cbv;
    cbv.Size = size;

    const size_t assignedOffset = uploadHeap->GetAssignedUploadOffset(size, 256);
    cbv.MappedGpuPtr = uploadHeap->GetMappedPointer(assignedOffset);

    cbv.HeapIndex = m_pHeap->GetNextDescriptorIdx(debugName);
    const auto handle = m_pHeap->GetDescriptorHandleAtIndex(cbv.HeapIndex);

    D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc = {};
    cbvDesc.BufferLocation = uploadHeap->GetUploadResource()->GetGPUVirtualAddress() + assignedOffset;
    cbvDesc.SizeInBytes = static_cast<uint32_t>(Align(size, 256));
    device->CreateConstantBufferView(&cbvDesc, handle);

    for (uint32_t i = m_cbvs.size(); i <= spaceIdx; i++)
        m_cbvs.push_back({});

    m_cbvs[spaceIdx].emplace_back(cbv);
}

void DescriptorSet::UpdateCBV(const uint32_t spaceIdx, const uint32_t cbvIdx, const void* data) const
{
    if (spaceIdx >= m_cbvs.size() || cbvIdx >= m_cbvs[spaceIdx].size())
        throw std::exception("You made a mistake :(");

    const CBV& cbv = m_cbvs[spaceIdx][cbvIdx];
    std::memcpy(cbv.MappedGpuPtr, data, cbv.Size);
}

void DescriptorSet::ReleaseCBVs()
{
    m_cbvs.clear();
}

void DescriptorSet::AddSRV(ID3D12Device* device, const uint32_t spaceIdx, D12Resource* d12Resource, const D3D12_SHADER_RESOURCE_VIEW_DESC& desc, const char* debugName)
{
    for (uint32_t i = m_srvs.size(); i <= spaceIdx; i++)
        m_srvs.push_back({});

    SRV srv;
    srv.HeapIndex = m_pHeap->GetNextDescriptorIdx(debugName);

    if (m_srvs[spaceIdx].size() > 0)
        CherryAssert(srv.HeapIndex - m_srvs[spaceIdx].back().HeapIndex == 1);

    ID3D12Resource* resource = d12Resource ? d12Resource->GetResource() : nullptr;

    const auto handle = m_pHeap->GetDescriptorHandleAtIndex(srv.HeapIndex);
    device->CreateShaderResourceView(resource, &desc, handle);

    srv.D12Resource = d12Resource;
    srv.Resource = resource;
    m_srvs[spaceIdx].emplace_back(srv);
}

void DescriptorSet::SetSRV(ID3D12Device* device, const uint32_t spaceIdx, const uint32_t srvIdx, D12Resource* d12Resource, const D3D12_SHADER_RESOURCE_VIEW_DESC& desc, const char* debugName)
{
    for (uint32_t i = m_srvs.size(); i <= spaceIdx; i++)
        m_srvs.push_back({});

    if (srvIdx > m_srvs[spaceIdx].size())
        throw std::exception("Invalid srv index");

    if (srvIdx == m_srvs[spaceIdx].size())
    {
        AddSRV(device, spaceIdx, d12Resource, desc, debugName);
        return;
    }

    ID3D12Resource* resource = d12Resource ? d12Resource->GetResource() : nullptr;

    SRV& srv = m_srvs[spaceIdx].at(srvIdx);
    if (resource && srv.Resource == resource)
        return;

    srv.D12Resource = d12Resource;
    srv.Resource = resource;
    const auto handle = m_pHeap->GetDescriptorHandleAtIndex(srv.HeapIndex);
    device->CreateShaderResourceView(resource, &desc, handle);
}

void DescriptorSet::SetSRV_Tex2D(ID3D12Device* device, const uint32_t spaceIdx, const uint32_t srvIdx, D12Resource* d12Resource, DXGI_FORMAT format)
{
    D3D12_SHADER_RESOURCE_VIEW_DESC desc = {};
    desc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
    desc.Format = format;
    desc.Texture2D.MipLevels = d12Resource ? d12Resource->GetDesc().MipLevels : 1;
    desc.Texture2D.MostDetailedMip = 0;
    desc.Texture2D.PlaneSlice = 0;
    desc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;

    const char* debugName = d12Resource && Config::GetSystem().DebugHeapEnabled ? d12Resource->GetName() : nullptr;
    SetSRV(device, spaceIdx, srvIdx, d12Resource, desc, debugName);
}

void DescriptorSet::SetSRV_Tex1D(ID3D12Device* device, const uint32_t spaceIdx, uint32_t srvIdx, D12Resource* d12Resource, DXGI_FORMAT format)
{
    D3D12_SHADER_RESOURCE_VIEW_DESC desc = {};
    desc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE1D;
    desc.Format = format;
    desc.Texture2D.MipLevels = d12Resource ? d12Resource->GetDesc().MipLevels : 1;
    desc.Texture2D.MostDetailedMip = 0;
    desc.Texture2D.PlaneSlice = 0;
    desc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;

    const char* debugName = d12Resource && Config::GetSystem().DebugHeapEnabled ? d12Resource->GetName() : nullptr;
    SetSRV(device, spaceIdx, srvIdx, d12Resource, desc, debugName);
}

void DescriptorSet::SetSRV_Buffer(ID3D12Device* device, const uint32_t spaceIdx, const uint32_t srvIdx, D12Resource* d12Resource, const uint32_t numElements, const size_t stride)
{
    D3D12_SHADER_RESOURCE_VIEW_DESC desc = {};
    desc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
    desc.Buffer.FirstElement = 0;
    desc.Buffer.NumElements = numElements;
    desc.Buffer.StructureByteStride = stride;
    desc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;

    const char* debugName = Config::GetSystem().DebugHeapEnabled ? d12Resource->GetName() : nullptr;
    SetSRV(device, spaceIdx, srvIdx, d12Resource, desc, debugName);
}

void DescriptorSet::SetSRV_RTAS(ID3D12Device* device, const uint32_t spaceIdx, const uint32_t srvIdx, const D12Resource* d12Resource)
{
    D3D12_SHADER_RESOURCE_VIEW_DESC desc = {};
    desc.ViewDimension = D3D12_SRV_DIMENSION_RAYTRACING_ACCELERATION_STRUCTURE;
    if (d12Resource)
        desc.RaytracingAccelerationStructure.Location = d12Resource->GetResource()->GetGPUVirtualAddress();
    desc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;

    const char* debugName = Config::GetSystem().DebugHeapEnabled && d12Resource ? d12Resource->GetName() : nullptr;
    SetSRV(device, spaceIdx, srvIdx, nullptr, desc, debugName);
}

void DescriptorSet::TransitionAllSRVToShaderResource(ID3D12GraphicsCommandList* cmdList) const
{
    for (const auto& space : m_srvs)
    {
        for (const auto& srv : space)
        {
            if (srv.D12Resource)
            {
                srv.D12Resource->Transition(cmdList, D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE);
            }
        }
    }
}

void DescriptorSet::AddUAV(ID3D12Device* device, const uint32_t spaceIdx, const D12Resource* d12Resource, const D3D12_UNORDERED_ACCESS_VIEW_DESC& desc)
{
    for (uint32_t i = m_uavs.size(); i <= spaceIdx; i++)
        m_uavs.push_back({});

    UAV uav;

    uav.HeapIndex = m_pHeap->GetNextDescriptorIdx("UAV");

    if (m_uavs[spaceIdx].size() > 0)
        CherryAssert(uav.HeapIndex - m_uavs[spaceIdx].back().HeapIndex == 1);

    ID3D12Resource* resource = d12Resource ? d12Resource->GetResource() : nullptr;

    if (resource)
    {
        const auto handle = m_pHeap->GetDescriptorHandleAtIndex(uav.HeapIndex);
        device->CreateUnorderedAccessView(resource, nullptr, &desc, handle);
    }

    m_uavs[spaceIdx].emplace_back(uav);
}

void DescriptorSet::SetUAV(ID3D12Device* device, const uint32_t spaceIdx, const uint32_t uavIdx, const D12Resource* d12Resource, const D3D12_UNORDERED_ACCESS_VIEW_DESC& desc)
{
    for (uint32_t i = m_uavs.size(); i <= spaceIdx; i++)
        m_uavs.push_back({});

    if (uavIdx > m_uavs[spaceIdx].size())
        throw std::exception("Invalid uav index");

    if (uavIdx == m_uavs[spaceIdx].size())
    {
        AddUAV(device, spaceIdx, d12Resource, desc);
        return;
    }

    ID3D12Resource* resource = d12Resource ? d12Resource->GetResource() : nullptr;

    if (resource)
    {
        const UAV& uav = m_uavs[spaceIdx].at(uavIdx);

        const auto handle = m_pHeap->GetDescriptorHandleAtIndex(uav.HeapIndex);
        device->CreateUnorderedAccessView(resource, nullptr, &desc, handle);
    }
}

void DescriptorSet::SetUAV_Tex2D(ID3D12Device* device, const uint32_t spaceIdx, const uint32_t uavIdx, const D12Resource* d12Resource, const DXGI_FORMAT format)
{
    D3D12_UNORDERED_ACCESS_VIEW_DESC desc = {};
    desc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
    desc.Format = format;
    desc.Texture2D.MipSlice = 0;
    desc.Texture2D.PlaneSlice = 0;

    SetUAV(device, spaceIdx, uavIdx, d12Resource, desc);
}

void DescriptorSet::SetUAV_Tex1D(ID3D12Device* device, const uint32_t spaceIdx, const uint32_t uavIdx, const D12Resource* d12Resource, const DXGI_FORMAT format)
{
    D3D12_UNORDERED_ACCESS_VIEW_DESC desc = {};
    desc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE1D;
    desc.Format = format;
    desc.Texture2D.MipSlice = 0;
    desc.Texture2D.PlaneSlice = 0;

    SetUAV(device, spaceIdx, uavIdx, d12Resource, desc);
}

void DescriptorSet::SetUAV_Buffer(ID3D12Device* device, const uint32_t spaceIdx, const uint32_t uavIdx, const D12Resource* d12Resource, const uint32_t numElements, const size_t stride)
{
    D3D12_UNORDERED_ACCESS_VIEW_DESC desc = {};
    desc.ViewDimension = D3D12_UAV_DIMENSION_BUFFER;
    desc.Buffer.FirstElement = 0;
    desc.Buffer.NumElements = numElements;
    desc.Buffer.StructureByteStride = stride;

    const char* debugName = Config::GetSystem().DebugHeapEnabled ? d12Resource->GetName() : nullptr;
    SetUAV(device, spaceIdx, uavIdx, d12Resource, desc);
}

void DescriptorSet::SetUAV_ByteAddressBuffer(ID3D12Device* device, const uint32_t spaceIdx, const uint32_t uavIdx, const D12Resource* d12Resource, const size_t size)
{
    D3D12_UNORDERED_ACCESS_VIEW_DESC desc = {};
    desc.ViewDimension = D3D12_UAV_DIMENSION_BUFFER;
    desc.Format = DXGI_FORMAT_R32_TYPELESS;
    desc.Buffer.FirstElement = 0;
    desc.Buffer.NumElements = size / 4; // Num 32-bit words
    desc.Buffer.StructureByteStride = 0;
    desc.Buffer.Flags = D3D12_BUFFER_UAV_FLAG_RAW;

    const char* debugName = Config::GetSystem().DebugHeapEnabled ? d12Resource->GetName() : nullptr;
    SetUAV(device, spaceIdx, uavIdx, d12Resource, desc);
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

    if (m_hasRootConstantsParam)
        paramIdx++;

    for (int spaceIdx = 0; spaceIdx < m_cbvs.size(); spaceIdx++)
    {
        CherryAssert(!m_cbvs[spaceIdx].empty());

        const CD3DX12_GPU_DESCRIPTOR_HANDLE cbvHandle(m_gpuHandle, m_cbvs[spaceIdx][0].HeapIndex, m_descriptorIncSize);
        if (isCompute)
            cmdList->SetComputeRootDescriptorTable(paramIdx, cbvHandle);
        else
            cmdList->SetGraphicsRootDescriptorTable(paramIdx, cbvHandle);
        paramIdx++;
    }

    for (int spaceIdx = 0; spaceIdx < m_srvs.size(); spaceIdx++)
    {
        CherryAssert(!m_srvs[spaceIdx].empty());

        const CD3DX12_GPU_DESCRIPTOR_HANDLE srvHandle(m_gpuHandle, m_srvs[spaceIdx][0].HeapIndex, m_descriptorIncSize);
        if (isCompute)
            cmdList->SetComputeRootDescriptorTable(paramIdx, srvHandle);
        else
            cmdList->SetGraphicsRootDescriptorTable(paramIdx, srvHandle);
        paramIdx++;
    }

    for (int spaceIdx = 0; spaceIdx < m_uavs.size(); spaceIdx++)
    {
        CherryAssert(!m_uavs[spaceIdx].empty());

        const CD3DX12_GPU_DESCRIPTOR_HANDLE uavHandle(m_gpuHandle, m_uavs[spaceIdx][0].HeapIndex, m_descriptorIncSize);
        if (isCompute)
            cmdList->SetComputeRootDescriptorTable(paramIdx, uavHandle);
        else
            cmdList->SetGraphicsRootDescriptorTable(paramIdx, uavHandle);
        paramIdx++;
    }

    if (m_hasBindlessParam)
        paramIdx++;
}

D12Resource* DescriptorSet::GetSRV(const uint32_t spaceIdx, const uint32_t srvIdx) const
{
    if (m_srvs.size() <= spaceIdx || m_srvs[spaceIdx].size() <= srvIdx)
        return nullptr;

    return m_srvs[spaceIdx][srvIdx].D12Resource;
}
