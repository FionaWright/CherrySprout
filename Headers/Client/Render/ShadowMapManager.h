#ifndef H_SHADOW_MAP_MANAGER_H
#define H_SHADOW_MAP_MANAGER_H

#include "HWI/D12Resource.h"
#include "HWI/DescriptorSet.h"
#include "HWI/Pipeline.h"
#include "HWI/RootSig.h"
#include "HWI/UploadHeap.h"
#include "Utils/CBVs.h"
#include "Utils/D3DUtils.h"

struct Scene;

#define SHADOW_MAP_FORMAT_DEPTH        DXGI_FORMAT_D32_FLOAT
#define SHADOW_MAP_FORMAT_DEPTH_SRV    DXGI_FORMAT_R32_FLOAT

class ShadowMapManager
{
public:
    void Init(ID3D12Device* device, Heap* heap, UploadHeap* uploadHeapCBV);
    void LoadSceneData(ID3D12Device* device, Scene* scene);

    void Render(D3D* d3d, ID3D12GraphicsCommandList* cmdList, Heap* heap, Scene* scene, XMFLOAT3 dirLightDir);
    std::vector<XMFLOAT2> GeneratePoissonSamples(uint32_t count, float minDistance = 0.15f);

    D12Resource* GetShadowMapDir() { return &m_shadowMapDir; }

    XMMATRIX GetShadowMatrix(uint32_t cascadeIdx) const { return m_V * m_P; }
    XMFLOAT2 GetShadowDimensions(uint32_t cascadeIdx) const { return XMFLOAT2(m_shadowMapDir.GetDesc().Width, m_shadowMapDir.GetDesc().Height); }

    static size_t GetCbvRequiredSize() { return Align(sizeof(CbvMatrices_VP), 256); }

private:
    RootConstants m_rootConstants;
    RootSig m_rootSig;
    DescriptorSet m_set;
    Pipeline m_pipeline;

    Heap m_heapDSV;
    uint32_t m_heapIdxDir = 0;

    D12Resource m_shadowMapDir;
    XMMATRIX m_V{};
    XMMATRIX m_P{};
    bool m_viewMatrixDirty = true;

    std::vector<D12Resource> m_shadowMapPunctuals;
};

#endif