//
// Created by fiona on 30/09/2025.
//

#include "System/pch.h"
#include "HWI/D12Resource.h"

#include "HWI/UploadHeap.h"
#include "Utils/Helper.h"

D12Resource::D12Resource(const ComPtr<ID3D12Resource>& resource, const D3D12_RESOURCE_STATES& initialState)
{
    m_resource = resource;
    m_currentState = initialState;

#ifdef _DEBUG
    m_initialized = true;
#endif
}

void D12Resource::Init(const char* name, ID3D12Device* device, const D3D12_RESOURCE_DESC& resourceDesc,
                       const D3D12_RESOURCE_STATES& initialState, const D3D12_CLEAR_VALUE* clearValue, const CD3DX12_HEAP_PROPERTIES& heapProp)
{
#ifdef _DEBUG
    m_name = name;
    assert(!m_initialized);
    m_initialized = true;
#endif

    V(device->CreateCommittedResource(&heapProp, D3D12_HEAP_FLAG_NONE, &resourceDesc, initialState, clearValue,
                                      IID_PPV_ARGS(&m_resource)));
    V(m_resource->SetName(stringToWString(name).c_str()));
    m_currentState = initialState;
    m_desc = resourceDesc;
}

void D12Resource::InitBuffer(const char* name, ID3D12Device* device, const size_t size,
                             const D3D12_RESOURCE_FLAGS flags, const bool readbackHeap)
{
    m_desc = CD3DX12_RESOURCE_DESC::Buffer(size, flags);
    const auto heapProp = readbackHeap ? CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_READBACK) : CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);

    Init(name, device, m_desc, D3D12_RESOURCE_STATE_COMMON, nullptr, heapProp);
}

void D12Resource::InitUpload(const char* name, ID3D12Device* device, const size_t uploadBufferSize)
{
    m_desc = CD3DX12_RESOURCE_DESC::Buffer(uploadBufferSize);
    Init(name, device, m_desc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD));
}

// TODO: Remove once I no longer need it for reference
// void D12Resource::InitRTAS(const char* name, ID3D12Device* device, const size_t size,
//                              const D3D12_RESOURCE_FLAGS flags)
// {
//     m_desc = CD3DX12_RESOURCE_DESC::Buffer(size, flags);
//     Init(name, device, m_desc, D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE);
// }

void D12Resource::UploadBuffer(ID3D12GraphicsCommandList* cmdList, UploadHeap* uploadHeap, const void* pData,
                             const size_t totalBytes)
{
    m_uploadBufferAssignedOffset = uploadHeap->GetAssignedUploadOffset(totalBytes, 4); // TODO: Alignment

    uint8_t* mappedPtr = uploadHeap->GetMappedPointer(m_uploadBufferAssignedOffset);
    memcpy(mappedPtr, pData, totalBytes);

    Transition(cmdList, D3D12_RESOURCE_STATE_COPY_DEST);
    cmdList->CopyBufferRegion(m_resource.Get(), 0, uploadHeap->GetUploadResource(), m_uploadBufferAssignedOffset, totalBytes);
}

void D12Resource::UploadTexture(ID3D12GraphicsCommandList* cmdList, UploadHeap* uploadHeap, const uint8_t* pData,
                                const size_t totalBytes,
                                const size_t rowPitch)
{
    assert(m_desc.DepthOrArraySize == 1);

    const UINT subresourceIndex = D3D12CalcSubresource(0, 0, 0, m_desc.MipLevels, m_desc.DepthOrArraySize);

    D3D12_SUBRESOURCE_DATA subresource = {};
    subresource.pData = pData;
    subresource.RowPitch = rowPitch;
    subresource.SlicePitch = totalBytes;

    const size_t requiredSize = GetRequiredIntermediateSize(m_resource.Get(), subresourceIndex, 1);
    m_uploadBufferAssignedOffset = uploadHeap->GetAssignedUploadOffset(requiredSize, D3D12_TEXTURE_DATA_PLACEMENT_ALIGNMENT);

    UpdateSubresources(cmdList, m_resource.Get(), uploadHeap->GetUploadResource(), m_uploadBufferAssignedOffset, subresourceIndex, 1, &subresource);
}

void D12Resource::UploadTextureArray(ID3D12GraphicsCommandList* cmdList, UploadHeap* uploadHeap, const uint8_t** pData,
                                const size_t totalBytesPerSlice,
                                const size_t rowPitch)
{
    assert(m_desc.MipLevels == 1);

    std::vector<D3D12_SUBRESOURCE_DATA> subresourceDatas(m_desc.DepthOrArraySize, {nullptr, (LONG_PTR)rowPitch, (LONG_PTR)totalBytesPerSlice});
    for (int a = 0; a < m_desc.DepthOrArraySize; a++)
    {
        subresourceDatas[a].pData = pData[a];
    }

    const UINT numSubresources = m_desc.MipLevels * m_desc.DepthOrArraySize;
    const size_t requiredSize = GetRequiredIntermediateSize(m_resource.Get(), 0, numSubresources);
    m_uploadBufferAssignedOffset = uploadHeap->GetAssignedUploadOffset(requiredSize, D3D12_TEXTURE_DATA_PLACEMENT_ALIGNMENT);

    UpdateSubresources(cmdList, m_resource.Get(), uploadHeap->GetUploadResource(), m_uploadBufferAssignedOffset, 0, numSubresources, subresourceDatas.data());
}

void D12Resource::Transition(ID3D12GraphicsCommandList* cmdList, const D3D12_RESOURCE_STATES& newState,
                             const UINT subresourceIdx)
{
    if (m_currentState == newState)
        return;

    const CD3DX12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(
        m_resource.Get(), m_currentState, newState, subresourceIdx);
    cmdList->ResourceBarrier(1, &barrier);
    m_currentState = newState;
}

void D12Resource::CopyTextureInto(ID3D12GraphicsCommandList* cmdList, ID3D12Resource* srcResource, const uint32_t dstX, const uint32_t dstY, const uint32_t dstZ, const D3D12_BOX* srcBox) const
{
    D3D12_TEXTURE_COPY_LOCATION srcLocation = {};
    srcLocation.pResource = srcResource;
    srcLocation.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
    srcLocation.SubresourceIndex = 0;

    D3D12_TEXTURE_COPY_LOCATION dstLocation = {};
    dstLocation.pResource = m_resource.Get();
    dstLocation.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
    dstLocation.SubresourceIndex = 0;

    cmdList->CopyTextureRegion(&dstLocation, dstX, dstY, dstZ, &srcLocation, srcBox);
}

UINT64 D12Resource::GetIntermediateSize() const
{
    const UINT numSubresources = m_desc.MipLevels * m_desc.DepthOrArraySize;
    return GetRequiredIntermediateSize(m_resource.Get(), 0, numSubresources);
}
