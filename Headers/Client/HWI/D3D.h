//
// Created by fionaw on 22/09/2025.
//

#ifndef PT_D3D_H
#define PT_D3D_H

#include "D12Resource.h"
#include "System/Config.h"

struct CommandAllocatorEntry
{
    UINT64 Fence;
    ComPtr<ID3D12CommandAllocator> Allocator;
};

struct CommandQueue
{
    ComPtr<ID3D12CommandQueue> Queue;
    ComPtr<ID3D12Fence> Fence;

    UINT64 NextFenceValue = 1;

    std::queue<CommandAllocatorEntry> AllocatorPool;
    std::queue<ComPtr<ID3D12GraphicsCommandList>> CommandListPool;
};

class D3D
{
public:
    ~D3D();
    void Init(size_t width, size_t height);
    ComPtr<ID3D12GraphicsCommandList> CreateCmdList(ID3D12CommandAllocator* allocator,
                                                    D3D12_COMMAND_LIST_TYPE type) const;

    ID3D12Device* GetDevice() const { return m_device.Get(); }
    UINT GetFrameIndex() const { return m_frameIndex; }

    D12Resource* GetRtv()
    {
        if (Config::GetRender().MsaaSampleCount > 1)
            return &m_msaaRTV;

        return GetSwapchainBackbuffer();
    }

    D12Resource* GetSwapchainBackbuffer()
    {
        return &m_rtvs[m_frameIndex];
    }

    D3D12_CPU_DESCRIPTOR_HANDLE GetRtvHandle() const
    {
        if (Config::GetRender().MsaaSampleCount > 1)
        {
            return CD3DX12_CPU_DESCRIPTOR_HANDLE(m_rtvHeap->GetCPUDescriptorHandleForHeapStart(),
                                             static_cast<int>(0), m_rtvDescriptorSize);
        }
        return GetBackbufferHandle();
    }

    D3D12_CPU_DESCRIPTOR_HANDLE GetBackbufferHandle() const
    {
        const uint32_t descriptorIdx = m_frameIndex + (Config::GetRender().MsaaSampleCount > 1 ? 1 : 0);
        return CD3DX12_CPU_DESCRIPTOR_HANDLE(m_rtvHeap->GetCPUDescriptorHandleForHeapStart(),
                                             static_cast<int>(descriptorIdx), m_rtvDescriptorSize);
    }

    void InitFrameResources(uint32_t width, uint32_t height);

    ID3D12Resource* GetCurrDSV() const { return m_depthStencilBuffer.Get(); }
    D3D12_CPU_DESCRIPTOR_HANDLE GetDsvHeapStart() const { return m_dsvHeap->GetCPUDescriptorHandleForHeapStart(); }
    UINT GetDsvDescriptorSize() const { return m_dsvDescriptorSize; }

    bool GetRayTracingSupported() const { return m_rayTracingSupported; }

    ComPtr<ID3D12CommandAllocator> CreateAllocator(D3D12_COMMAND_LIST_TYPE type) const;
    ComPtr<ID3D12GraphicsCommandList> GetAvailableCmdList(D3D12_COMMAND_LIST_TYPE type);
    void DestroyAllCmdListsAndAllocators();
    void ExecuteCommandList(ID3D12GraphicsCommandList* cmdList);
    void Present();
    UINT64 Signal(D3D12_COMMAND_LIST_TYPE type);
    void Flush();
    void WaitForSignal(UINT64 fence, D3D12_COMMAND_LIST_TYPE type);
    bool IsFenceComplete(UINT64 fenceVal, D3D12_COMMAND_LIST_TYPE type);

private:
    CommandQueue& getQueue(D3D12_COMMAND_LIST_TYPE type);

    // Pipeline objects.
    ComPtr<IDXGISwapChain3> m_swapChain;
    ComPtr<ID3D12Device> m_device;
    ComPtr<IDXGIFactory4> m_factory;

    D12Resource m_rtvs[NUM_FRAMES_IN_FLIGHT];
    ComPtr<ID3D12Resource> m_depthStencilBuffer;
    D12Resource m_msaaRTV;

    ComPtr<ID3D12DescriptorHeap> m_rtvHeap, m_dsvHeap;
    UINT m_rtvDescriptorSize = 0, m_dsvDescriptorSize = 0;

    CommandQueue m_commandQueueDirect;
    CommandQueue m_commandQueueCompute;
    CommandQueue m_commandQueueCopy;

    bool m_useWarpDevice = false;
    bool m_tearingSupport = false;
    bool m_rayTracingSupported = false;

    // Synchronization objects.
    UINT m_frameIndex = 0;
    HANDLE m_fenceEvent = {};
    UINT64 m_frameBufferFences[NUM_FRAMES_IN_FLIGHT] = {};

    // Debugging
    ComPtr<ID3D12InfoQueue1> m_infoQueue;
    std::ofstream m_logFile;
};

#endif //PT_D3D_H
