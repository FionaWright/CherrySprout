//
// Created by fiona on 30/09/2025.
//

#ifndef PT_D12RESOURCE_H
#define PT_D12RESOURCE_H

class UploadHeap;

class D12Resource
{
public:
    D12Resource() {}
    D12Resource(const ComPtr<ID3D12Resource>& resource, const D3D12_RESOURCE_STATES& initialState);

    void Init(const char* name, ID3D12Device* device, const D3D12_RESOURCE_DESC& resourceDesc,
              const D3D12_RESOURCE_STATES& initialState, const D3D12_CLEAR_VALUE* clearValue = nullptr,
              const CD3DX12_HEAP_PROPERTIES& heapProp = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT));
    void InitBuffer(const char* name, ID3D12Device* device, size_t size,
                    D3D12_RESOURCE_FLAGS flags = D3D12_RESOURCE_FLAG_NONE, bool readbackHeap = false);
    void InitUpload(const char* name, ID3D12Device* device, size_t uploadBufferSize);
    //void InitRTAS(const char* name, ID3D12Device* device, size_t size, D3D12_RESOURCE_FLAGS flags);

    void UploadBuffer(ID3D12GraphicsCommandList* cmdList, UploadHeap* uploadHeap, const void* pData, size_t totalBytes);
    void UploadTexture(ID3D12GraphicsCommandList* cmdList, UploadHeap* uploadHeap, const uint8_t* pData,
                       size_t totalBytes, size_t rowPitch);
    void UploadTextureArray(ID3D12GraphicsCommandList* cmdList, UploadHeap* uploadHeap, const uint8_t** pData,
                       size_t totalBytesPerSlice, size_t rowPitch);

    void Transition(ID3D12GraphicsCommandList* cmdList, const D3D12_RESOURCE_STATES& newState,
                    UINT subresourceIdx = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES);
    void CopyTextureInto(ID3D12GraphicsCommandList* cmdList, ID3D12Resource* srcResource, uint32_t dstX = 0,
                         uint32_t dstY = 0,
                         uint32_t dstZ = 0, const D3D12_BOX* srcBox = nullptr) const;

    UINT64 GetIntermediateSize() const;

    ID3D12Resource* GetResource() const { return m_resource.Get(); }
    D3D12_RESOURCE_STATES GetCurrentState() const { return m_currentState; }
    D3D12_RESOURCE_DESC GetDesc() const { return m_desc; }
    [[nodiscard]] const std::string& GetName() const { return m_name; }

private:
    ComPtr<ID3D12Resource> m_resource;
    D3D12_RESOURCE_DESC m_desc = {};
    D3D12_RESOURCE_STATES m_currentState = {};
    size_t m_uploadBufferAssignedOffset = 0;

#ifdef _DEBUG
    std::string m_name;
    bool m_initialized = false;
#endif
};

#endif //PT_D12RESOURCE_H
