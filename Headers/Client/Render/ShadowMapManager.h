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

struct ShadowMapConfig
{
    uint32_t CascadeCount;

    float Width = 1.0f;
    float Height = 1.0f;
    float Near = 0.01f;
    float Far = 100.0f;
    float DirDist = 8.0f;

    XMFLOAT3 DirLightDir;
};

struct ShadowMap
{
    D12Resource Resource;
    std::vector<uint32_t> DsvHeapIndices;
    std::vector<XMMATRIX> Vs;
    XMMATRIX P;
    uint32_t HeapIdx = UINT_MAX;
};

class ShadowMapManager
{
public:
    void Init(ID3D12Device* device, Heap* heap, UploadHeap* uploadHeapCBV);
    void LoadSceneData(D3D* d3d, Heap* heap, Scene* scene);

    void Render(D3D* d3d, ID3D12GraphicsCommandList* cmdList, const Heap* heap, Scene* scene, const ShadowMapConfig& config);
    void GeneratePoissonSamples(uint32_t count, float minDistance = 0.15f);
    const std::vector<XMFLOAT2>& GetPoissonSamples() const { return m_poissonDiscSamples; }

    D12Resource* GetShadowMapDir() { return &m_shadowMapDir.Resource; }
    D12Resource* GetShadowMapPuncInfo() { return &m_shadowMapPuncInfoBuffer; }
    uint32_t GetHeapBaseDistant() const { return m_heapBaseDistant; }
    uint32_t GetHeapBasePoint() const { return m_heapBasePoint; }

    XMMATRIX GetShadowMatrix(uint32_t cascadeIdx) const { return m_shadowMapDir.Vs[0] * m_shadowMapDir.P; }
    XMFLOAT2 GetShadowDimensions(uint32_t cascadeIdx) const { return XMFLOAT2(m_shadowMapDir.Resource.GetDesc().Width, m_shadowMapDir.Resource.GetDesc().Height); }

    void MarkMatricesDirty() { m_matricesDirty = true; }

    static size_t GetCbvRequiredSize() { return 0; }

private:
    XMMATRIX getViewMatrixDistant(XMFLOAT3 direction, float distance) const;
    XMMATRIX getViewMatrixPoint(XMFLOAT3 position, XMFLOAT3 direction) const;
    XMMATRIX getProjectionMatrixDistant(float width, float height, float nearPlane, float farPlane);
    XMMATRIX getProjectionMatrixPoint(float width, float height, float nearPlane, float farPlane);
    void renderSceneObjectsToBoundMap(ID3D12GraphicsCommandList* cmdList, const Scene* scene, CbvShadowMap_PerInstance& cbv) const;
    void renderDistant(ID3D12GraphicsCommandList* cmdList, const Scene* scene, ShadowMap* shadowMap) const;
    void renderPoint(ID3D12GraphicsCommandList* cmdList, const Scene* scene, ShadowMap* shadowMap, XMFLOAT3 position, float farPlane) const;

    RootConstants m_rootConstants;
    RootSig m_rootSig;
    DescriptorSet m_set;
    Pipeline m_pipelineDistant;
    Pipeline m_pipelinePoint;

    Heap m_heapDSV;

    ShadowMap m_shadowMapDir{};

    D12Resource m_shadowMapPuncInfoBuffer;

    bool m_matricesDirty = true;

    std::vector<ShadowMap> m_shadowMapPunctuals;
    uint32_t m_heapBaseDistant = UINT_MAX;
    uint32_t m_heapBasePoint = UINT_MAX;

    std::vector<XMFLOAT2> m_poissonDiscSamples;
};

#endif