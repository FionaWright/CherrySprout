//
// Created by fiona on 30/09/2025.
//

#ifndef PT_D12RESOURCE_H
#define PT_D12RESOURCE_H

class UploadHeap;

class D12Resource
{
public:
    D12Resource()
    {
    }

    D12Resource(const ComPtr<ID3D12Resource>& resource, const D3D12_RESOURCE_STATES& initialState,
                const D3D12_RESOURCE_DESC& desc);

    void Init(const char* name, ID3D12Device* device, const D3D12_RESOURCE_DESC& resourceDesc,
              const D3D12_RESOURCE_STATES& initialState, const D3D12_CLEAR_VALUE* clearValue = nullptr,
              const CD3DX12_HEAP_PROPERTIES& heapProp = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT));
    void Init_Buffer(const char* name, ID3D12Device* device, size_t size,
                     D3D12_RESOURCE_FLAGS flags = D3D12_RESOURCE_FLAG_NONE, bool readbackHeap = false,
                     D3D12_RESOURCE_STATES initialState = D3D12_RESOURCE_STATE_COMMON);
    void Init_Tex2D(const char* name, ID3D12Device* device, uint32_t width, uint32_t height, uint32_t depth,
                    DXGI_FORMAT format, D3D12_RESOURCE_FLAGS flags = D3D12_RESOURCE_FLAG_NONE,
                    D3D12_RESOURCE_STATES initialState = D3D12_RESOURCE_STATE_COMMON, const XMFLOAT4* clearValue = nullptr);
    void Init_Depth2D(const char* name, ID3D12Device* device, uint32_t width, uint32_t height,
                      D3D12_RESOURCE_FLAGS flags,
                      D3D12_RESOURCE_STATES initialState);
    void Init_Tex1D(const char* name, ID3D12Device* device, uint32_t width, uint32_t height, uint32_t depth,
                    DXGI_FORMAT format, D3D12_RESOURCE_FLAGS flags = D3D12_RESOURCE_FLAG_NONE,
                    D3D12_RESOURCE_STATES initialState = D3D12_RESOURCE_STATE_COMMON);
    void Init_Upload(const char* name, ID3D12Device* device, size_t uploadBufferSize);

    void UploadBuffer(ID3D12GraphicsCommandList* cmdList, UploadHeap* uploadHeap, const void* pData, size_t totalBytes);
    void UploadTexture(ID3D12GraphicsCommandList* cmdList, UploadHeap* uploadHeap, const uint8_t* pData,
                       size_t totalBytes, size_t rowPitch);
    void UploadTextureArray(ID3D12GraphicsCommandList* cmdList, UploadHeap* uploadHeap, const uint8_t** pData,
                            size_t totalBytesPerSlice, size_t rowPitch);

    void Transition(ID3D12GraphicsCommandList* cmdList, const D3D12_RESOURCE_STATES& newState,
                    UINT subresourceIdx = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES);
    void UavBarrier(ID3D12GraphicsCommandList* cmdList) const;
    void CopyTextureInto(ID3D12GraphicsCommandList* cmdList, ID3D12Resource* srcResource, uint32_t dstX = 0,
                         uint32_t dstY = 0,
                         uint32_t dstZ = 0, const D3D12_BOX* srcBox = nullptr) const;

    void Readback(void* dst) const;

    UINT64 GetIntermediateSize() const;
    void Release();
    void Reset();
    bool IsInitialized() const { return m_resource != nullptr; }

    ID3D12Resource* GetResource() const { return m_resource.Get(); }
    D3D12_RESOURCE_STATES GetCurrentState() const { return m_currentState; }
    D3D12_RESOURCE_DESC GetDesc() const { return m_desc; }

#ifdef _DEBUG
    [[nodiscard]] const char* GetName() const { return m_name.c_str(); }
    void SetName(const char* name) { m_name = name; }
#else
    [[nodiscard]] const char* GetName() const { return nullptr; }
    void SetName(const char* name) {}
#endif

private:
    ComPtr<ID3D12Resource> m_resource = nullptr;
    D3D12_RESOURCE_DESC m_desc = {};
    D3D12_RESOURCE_STATES m_currentState = {};
    size_t m_uploadBufferAssignedOffset = 0;

#ifdef _DEBUG
    std::string m_name = "";
    bool m_initialized = false;
#endif
};

#endif //PT_D12RESOURCE_H
