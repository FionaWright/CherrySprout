//
// Created by fiona on 06/10/2025.
//

#ifndef PT_DESCRIPTOR_SET_H
#define PT_DESCRIPTOR_SET_H

#include <d3d12.h>
#include <string>
#include <vector>

#include "HWI/D12Resource.h"

class TLAS;
class Heap;

struct CBV
{
    uint32_t HeapIndex = 0;
    size_t Size = 0;

    uint8_t* MappedGpuPtr = nullptr;
};

struct SRV
{
    uint32_t HeapIndex = 0;
    D12Resource* D12Resource = nullptr;
};

struct UAV
{
    uint32_t HeapIndex = 0;
};

class DescriptorSet
{
public:
    ~DescriptorSet();
    void Init(Heap* heap, bool hasBindlessParam = false, bool hasRootConstantsParam = false);
    const char* GetName() const { return m_name.c_str(); }
    void SetName(const char* str) { m_name = str; }

    void AddCBV(ID3D12Device* device, size_t size, UploadHeap* uploadHeap, const char* debugName = nullptr);
    void UpdateCBV(uint32_t idx, const void* data) const;

    void AddSRV(ID3D12Device* device, D12Resource* d12Resource, const D3D12_SHADER_RESOURCE_VIEW_DESC& desc, const char* debugName);
    void SetSRV(ID3D12Device* device, uint32_t srvIdx, D12Resource* d12Resource, const D3D12_SHADER_RESOURCE_VIEW_DESC& desc, const char* debugName = nullptr);
    void SetSRV_Tex2D(ID3D12Device* device, uint32_t srvIdx, D12Resource* d12Resource, DXGI_FORMAT format);
    void SetSRV_Buffer(ID3D12Device* device, uint32_t srvIdx, D12Resource* d12Resource, uint32_t numElements, size_t stride);
    void SetSRV_RTAS(ID3D12Device* device, uint32_t srvIdx, const D12Resource* d12Resource);

    void TransitionAllSRVToShaderResource(ID3D12GraphicsCommandList* cmdList) const;

    void AddUAV(ID3D12Device* device, const D12Resource* d12Resource, const D3D12_UNORDERED_ACCESS_VIEW_DESC& desc);
    void SetUAV(ID3D12Device* device, uint32_t uavIdx, const D12Resource* d12Resource, const D3D12_UNORDERED_ACCESS_VIEW_DESC& desc);
    void SetUAV_Tex2D(ID3D12Device* device, uint32_t uavIdx, const D12Resource* d12Resource, DXGI_FORMAT format);
    void SetUAV_Buffer(ID3D12Device* device, uint32_t uavIdx, D12Resource* d12Resource, uint32_t numElements,
                       size_t stride);

    void SetDescriptorTables_Graphics(ID3D12GraphicsCommandList* cmdList) const;
    void SetDescriptorTables_Compute(ID3D12GraphicsCommandList* cmdList) const;

private:
    void setDescriptorTables(ID3D12GraphicsCommandList* cmdList, bool isCompute) const;

    std::string m_name = "Descriptor Set";

    Heap* m_pHeap = nullptr;

    D3D12_CPU_DESCRIPTOR_HANDLE m_cpuHandle = {};
    D3D12_GPU_DESCRIPTOR_HANDLE m_gpuHandle = {};
    uint32_t m_descriptorIncSize = 0;

    bool m_hasBindlessParam = false;
    bool m_hasRootConstantsParam = false;

    std::vector<CBV> m_cbvs = {};
    std::vector<SRV> m_srvs = {};
    std::vector<SRV> m_srvsTextures = {};
    std::vector<UAV> m_uavs = {};
};


#endif //PT_MATERIAL_H
