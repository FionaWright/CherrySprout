//
// Created by fiona on 06/10/2025.
//

#include "System/pch.h"
#include "HWI/Heap.h"

#include "HWI/D12Resource.h"
#include "Utils/Helper.h"
#include "System/Config.h"

void Heap::Init(const char* name, ID3D12Device* device, const size_t numDescriptors, const D3D12_DESCRIPTOR_HEAP_TYPE type)
{
    m_name = name;
    m_type = type;
    m_descriptorIncSize = device->GetDescriptorHandleIncrementSize(m_type);
    m_heapSize = numDescriptors;

    m_baseBindlessTex = static_cast<size_t>(static_cast<float>(m_heapSize) * 0.65f);
    m_currentHeapIndexBindlessTex = m_baseBindlessTex; // Should I give more control to the user?

    D3D12_DESCRIPTOR_HEAP_DESC desc = {};
    desc.Type = m_type;
    desc.NumDescriptors = numDescriptors;
    desc.Flags = m_type == D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV ? D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE : D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
    desc.NodeMask = 0;

    V(device->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&m_heapResource)));
    V(m_heapResource->SetName(L"Heap"));

#ifdef _DEBUG
    if (Config::GetSystem().DebugHeapEnabled)
    {
        m_debugDescriptorNames.resize(m_baseBindlessTex);
        m_debugDescriptorNamesBindless.resize(m_heapSize - m_baseBindlessTex);
    }
#endif
}

CD3DX12_CPU_DESCRIPTOR_HANDLE Heap::GetDescriptorHandleAtIndex(const uint32_t idx) const
{
    return CD3DX12_CPU_DESCRIPTOR_HANDLE(m_heapResource->GetCPUDescriptorHandleForHeapStart(), idx,
                                                   m_descriptorIncSize);
}

uint32_t Heap::GetNextDescriptorIdx(const char* debugName)
{
    if (m_currentHeapIndex >= m_baseBindlessTex)
        throw std::exception("Heap is too smol :(");

    const uint32_t idx = m_currentHeapIndex;
    m_currentHeapIndex++;

#ifdef _DEBUG
    if (debugName && Config::GetSystem().DebugHeapEnabled)
    {
        m_debugDescriptorNames.at(idx) = _strdup(debugName);
    }
#endif

    return idx;
}

uint32_t Heap::GetNextDescriptorIdx_Bindless(const char* debugName)
{
    if (m_currentHeapIndexBindlessTex >= m_heapSize)
        throw std::exception("Heap is too smol :(");

    const uint32_t idx = m_currentHeapIndexBindlessTex;
    m_currentHeapIndexBindlessTex++;

#ifdef _DEBUG
    if (debugName && Config::GetSystem().DebugHeapEnabled)
    {
        m_debugDescriptorNamesBindless.at(idx - m_baseBindlessTex) = _strdup(debugName);
    }
#endif

    return idx;
}

uint32_t Heap::AddBindlessTexture2D(ID3D12Device* device, D12Resource* resource, DXGI_FORMAT format)
{
    D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
    srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
    srvDesc.Texture2D.MipLevels = 1;
    srvDesc.Texture2D.MostDetailedMip = 0;
    srvDesc.Texture2D.PlaneSlice = 0;
    srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    srvDesc.Format = format;

    uint32_t idx = GetNextDescriptorIdx_Bindless(resource->GetName().c_str());
    const auto handle = GetDescriptorHandleAtIndex(idx);

    device->CreateShaderResourceView(resource->GetResource(), &srvDesc, handle);

    uint32_t normalizedIdx = idx - GetBindlessTexBase();
    return normalizedIdx;
}

void Heap::Bind(ID3D12GraphicsCommandList* cmdList) const
{
    ID3D12DescriptorHeap* heap = m_heapResource.Get();
    cmdList->SetDescriptorHeaps(1, &heap);
}

void Heap::PrintHeapInfo() const
{
    const uint32_t totalRegularDescriptors = m_currentHeapIndex;
    const uint32_t totalBindlessDescriptors = m_currentHeapIndexBindlessTex - m_baseBindlessTex;
    CherryPrint("[" << m_name << "] Total Binded Descriptors: " << totalRegularDescriptors << " , Total Bindless Descriptors: " << totalBindlessDescriptors);
    CherryPrint("[" << m_name << "] Total Descriptors: " << totalRegularDescriptors + totalBindlessDescriptors << "/" << m_heapSize);
}
