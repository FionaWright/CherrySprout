//
// Created by fiona on 06/10/2025.
//

#include "System/pch.h"
#include "HWI/Heap.h"

#include "HWI/D12Resource.h"
#include "Utils/Helper.h"
#include "System/Config.h"

void Heap::Init(const char* name, ID3D12Device* device, const size_t numDescriptors, const size_t numSceneTextureDescriptors, const D3D12_DESCRIPTOR_HEAP_TYPE type)
{
    m_name = name;
    m_type = type;
    m_descriptorIncSize = device->GetDescriptorHandleIncrementSize(m_type);
    m_heapSize = numDescriptors + numSceneTextureDescriptors;

    m_baseSceneTextures = numDescriptors;
    m_currentHeapIndexSceneTextures = m_baseSceneTextures;

    D3D12_DESCRIPTOR_HEAP_DESC desc = {};
    desc.Type = m_type;
    desc.NumDescriptors = m_heapSize;
    desc.Flags = m_type == D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV ? D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE : D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
    desc.NodeMask = 0;

    V(device->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&m_heapResource)));
    V(m_heapResource->SetName(L"Heap"));

#ifdef _DEBUG
    if (Config::GetSystem().DebugHeapEnabled)
    {
        m_debugDescriptorNames.resize(numDescriptors);
        m_debugDescriptorNamesBindless.resize(numSceneTextureDescriptors);
    }
#endif
}

CD3DX12_CPU_DESCRIPTOR_HANDLE Heap::GetDescriptorHandleAtIndex(const uint32_t idx) const
{
    return CD3DX12_CPU_DESCRIPTOR_HANDLE(m_heapResource->GetCPUDescriptorHandleForHeapStart(), idx, m_descriptorIncSize);
}

uint32_t Heap::GetNextDescriptorIdx(const char* debugName)
{
    if (m_currentHeapIndex >= m_baseSceneTextures)
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

uint32_t Heap::GetNextDescriptorIdx_SceneTexture(const char* debugName)
{
    if (m_currentHeapIndexSceneTextures >= m_heapSize)
        throw std::exception("Heap is too smol :(");

    const uint32_t idx = m_currentHeapIndexSceneTextures;
    m_currentHeapIndexSceneTextures++;

#ifdef _DEBUG
    if (debugName && Config::GetSystem().DebugHeapEnabled)
    {
        m_debugDescriptorNamesBindless.at(idx - m_baseSceneTextures) = _strdup(debugName);
    }
#endif

    return idx;
}

uint32_t Heap::AddSRV_SceneTexture(ID3D12Device* device, const D12Resource* resource)
{
    D3D12_SHADER_RESOURCE_VIEW_DESC desc = {};
    desc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
    desc.Format = resource->GetDesc().Format;
    desc.Texture2D.MipLevels = 1;
    desc.Texture2D.MostDetailedMip = 0;
    desc.Texture2D.PlaneSlice = 0;
    desc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;

    const uint32_t idx = GetNextDescriptorIdx_SceneTexture(resource->GetName());
    const auto handle = GetDescriptorHandleAtIndex(idx);

    device->CreateShaderResourceView(resource->GetResource(), &desc, handle);

    const uint32_t normalizedIdx = idx - GetBindlessTexBase();
    return normalizedIdx;
}

void Heap::FreeSceneTextures() // Always do on scene change
{
    m_currentHeapIndexSceneTextures = m_baseSceneTextures;
#ifdef _DEBUG
    m_debugDescriptorNamesBindless.clear();
#endif
}

void Heap::Bind(ID3D12GraphicsCommandList* cmdList) const
{
    ID3D12DescriptorHeap* heap = m_heapResource.Get();
    cmdList->SetDescriptorHeaps(1, &heap);
}

void Heap::BindSceneTextures_Graphics(ID3D12GraphicsCommandList* cmdList) const
{
    const CD3DX12_GPU_DESCRIPTOR_HANDLE bindlessHandle(GetGPUHandle(), GetBindlessTexBase(), GetIncrementSize());
    cmdList->SetGraphicsRootDescriptorTable(2, bindlessHandle);
}

void Heap::BindSceneTextures_Compute(ID3D12GraphicsCommandList* cmdList) const
{
    const CD3DX12_GPU_DESCRIPTOR_HANDLE bindlessHandle(GetGPUHandle(), GetBindlessTexBase(), GetIncrementSize());
    cmdList->SetComputeRootDescriptorTable(2, bindlessHandle);
}

void Heap::PrintHeapInfo() const
{
    const uint32_t totalRegularDescriptors = m_currentHeapIndex;
    const uint32_t totalSceneTexDescriptors = m_currentHeapIndexSceneTextures - m_baseSceneTextures;
    CherryPrint("[" << m_name << "] Total Binded Descriptors: " << totalRegularDescriptors << " , Total Bindless Descriptors: " << totalSceneTexDescriptors);
    CherryPrint("[" << m_name << "] Total Descriptors: " << totalRegularDescriptors + totalSceneTexDescriptors << "/" << m_heapSize);
}
