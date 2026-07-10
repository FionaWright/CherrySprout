//
// Created by fiona on 06/10/2025.
//

#ifndef PT_HEAP_H
#define PT_HEAP_H

#include <complex.h>

class D12Resource;
using Microsoft::WRL::ComPtr;

class Heap
{
public:
    void Init(const char* name, ID3D12Device* device, size_t numDescriptors, size_t numSceneTextureDescriptors, D3D12_DESCRIPTOR_HEAP_TYPE
              type);
    CD3DX12_CPU_DESCRIPTOR_HANDLE GetDescriptorHandleAtIndex(uint32_t idx) const;
    uint32_t GetNextDescriptorIdx(const char* debugName = nullptr);

    uint32_t GetNextDescriptorIdx_SceneTexture(const char* debugName = nullptr);
    uint32_t AddSRV_SceneTexture(ID3D12Device* device, const D12Resource* resource);
    void FreeSceneTextures();
    [[nodiscard]] uint32_t GetBindlessTexBase() const { return m_baseSceneTextures; }

    void Bind(ID3D12GraphicsCommandList* cmdList) const;
    void BindSceneTextures_Graphics(ID3D12GraphicsCommandList* cmdList) const;
    void BindSceneTextures_Compute(ID3D12GraphicsCommandList* cmdList) const;
    void PrintHeapInfo() const;

    [[nodiscard]] D3D12_CPU_DESCRIPTOR_HANDLE GetCPUHandle() const { return m_heapResource->GetCPUDescriptorHandleForHeapStart(); }
    [[nodiscard]] D3D12_GPU_DESCRIPTOR_HANDLE GetGPUHandle() const { return m_heapResource->GetGPUDescriptorHandleForHeapStart(); }
    [[nodiscard]] uint32_t GetIncrementSize() const { return m_descriptorIncSize; }
    [[nodiscard]] D3D12_DESCRIPTOR_HEAP_TYPE GetType() const { return m_type; }
    [[nodiscard]] const std::vector<const char*>& GetDebugDescriptorList() const { return m_debugDescriptorNames; }
    [[nodiscard]] const std::vector<const char*>& GetDebugDescriptorListBindless() const { return m_debugDescriptorNamesBindless; }
    [[nodiscard]] uint32_t GetCurrBindedDescriptorCount() const { return m_currentHeapIndex; }
    [[nodiscard]] uint32_t GetCurrBindlessDescriptorCount() const { return m_currentHeapIndexSceneTextures - m_baseSceneTextures; }

private:
    std::string m_name;
    ComPtr<ID3D12DescriptorHeap> m_heapResource;
    uint32_t m_descriptorIncSize = 0;
    D3D12_DESCRIPTOR_HEAP_TYPE m_type = {};

    // Used only when flag --debugHeap given
    std::vector<const char*> m_debugDescriptorNames = {};
    std::vector<const char*> m_debugDescriptorNamesBindless = {};

    size_t m_baseSceneTextures = 0;
    size_t m_currentHeapIndex = 0;
    size_t m_currentHeapIndexSceneTextures = 0;
    size_t m_heapSize = 0;
};


#endif //PT_HEAP_H