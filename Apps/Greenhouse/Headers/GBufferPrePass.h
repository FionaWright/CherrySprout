#ifndef H_GBUFFER_PRE_PASS_H
#define H_GBUFFER_PRE_PASS_H

#include <d3d12.h>

#include "HWI/D12Resource.h"
#include "HWI/DescriptorSet.h"
#include "HWI/Heap.h"
#include "HWI/Pipeline.h"
#include "HWI/RootSig.h"

struct Scene;
class D3D;
class Heap;

#define GBUFFER_FORMAT_MAT_IDX      DXGI_FORMAT_R8_UINT
#define GBUFFER_FORMAT_NORMALS      DXGI_FORMAT_R10G10B10A2_UNORM // TODO: Change to Rg16f
#define GBUFFER_FORMAT_UV_MV        DXGI_FORMAT_R16G16B16A16_FLOAT
#define GBUFFER_FORMAT_DEPTH        DXGI_FORMAT_D32_FLOAT
#define GBUFFER_FORMAT_DEPTH_SRV    DXGI_FORMAT_R32_FLOAT

class GBufferPrePass
{
public:
    void Init(const D3D* d3d, Heap* heap, UploadHeap* uploadHeapCBV);
    void LoadSceneData(D3D* d3d, Scene* scene);
    void Render(D3D* d3d, ID3D12GraphicsCommandList* cmdList, Scene* scene, const Heap* heap, const XMMATRIX& V, const XMMATRIX& P);

    [[nodiscard]] D12Resource* GetGBufferMaterialIdx() { return &m_gbufferTexMaterialIdx; }
    [[nodiscard]] D12Resource* GetGBufferNormals() { return &m_gbufferTexNormals; }
    [[nodiscard]] D12Resource* GetGBufferDepth() { return &m_gbufferTexDepth; }
    [[nodiscard]] D12Resource* GetGBufferUvMv() { return &m_gbufferTexUvMv; }

private:
    uint32_t createRTV(ID3D12Device* device, const D12Resource* resource);
    uint32_t createDSV(ID3D12Device* device, const D12Resource* resource);

    Heap m_heapRTV;
    Heap m_heapDSV;
    uint32_t m_heapIdxMatIdx = 0;
    uint32_t m_heapIdxNormals = 0;
    uint32_t m_heapIdxUvMv = 0;
    uint32_t m_heapIdxDepth = 0;
    std::vector<XMFLOAT4> m_rtvClearValues;

    D12Resource m_gbufferTexMaterialIdx;
    D12Resource m_gbufferTexNormals;
    D12Resource m_gbufferTexDepth;
    D12Resource m_gbufferTexUvMv;

    Pipeline m_pipeline;
    RootSig m_rootSig;
    DescriptorSet m_descriptorSet;
    RootConstants m_rootConstants;
};

#endif