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

class ShadowMapManager
{
public:
    void Init(ID3D12Device* device, Heap* heap, UploadHeap* uploadHeapCBV);
    void LoadSceneData(ID3D12Device* device, Scene* scene);

    void Render(D3D* d3d, ID3D12GraphicsCommandList* cmdList, Heap* heap, Scene* scene, XMFLOAT3 dirLightDir, const XMMATRIX& P);

    static size_t GetCbvRequiredSize() { return Align(sizeof(CbvMatrices_VP), 256); }

private:
    RootConstants m_rootConstants;
    RootSig m_rootSig;
    DescriptorSet m_set;
    Pipeline m_pipeline;

    Heap m_heapDSV;
    uint32_t m_heapIdxDepth = 0;

    D12Resource m_shadowMapDir;
    XMMATRIX m_P{};

    std::vector<D12Resource> m_shadowMapPunctuals;
};

#endif