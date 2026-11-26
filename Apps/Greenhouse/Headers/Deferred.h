//
// Created by fiona on 01/06/2026.
//

#ifndef CHERRYSPROUT_DEFERRED_H
#define CHERRYSPROUT_DEFERRED_H

#include "IRenderBackend.h"
#include "HWI/DescriptorSet.h"
#include "HWI/RootSig.h"
#include "HWI/Pipeline.h"
#include "HWI/RtasBuilder.h"
#include "Render/RestirManager.h"
#include "Render/ShadowMapManager.h"
#include "Render/Skybox.h"
#include "Scene/Material.h"
#include "Utils/CBVs.h"

#define DEFERRED_GBUFFER_FORMAT_MAT_IDX      DXGI_FORMAT_R8_UINT
#define DEFERRED_GBUFFER_FORMAT_NORMALS_UV   DXGI_FORMAT_R16G16B16A16_FLOAT
#define DEFERRED_GBUFFER_FORMAT_MV           DXGI_FORMAT_R16G16B16A16_FLOAT
#define DEFERRED_GBUFFER_FORMAT_EMISSION     DXGI_FORMAT_R16G16B16A16_FLOAT
#define DEFERRED_GBUFFER_FORMAT_DEPTH        DXGI_FORMAT_D32_FLOAT
#define DEFERRED_GBUFFER_FORMAT_DEPTH_SRV    DXGI_FORMAT_R32_FLOAT

struct Scene;
struct TimeArgs;

class Deferred final : public IRenderBackend
{
public:
    void Init(D3D* d3d, Heap* heap, UploadHeap* uploadHeapCBV) override;
    void LoadSceneData(D3D* d3d, Scene* scene, Heap* heap, UploadHeap* uploadHeapCBV, EnvironmentMap* envMap, LightImportanceSampler*
                       lightImportanceSampler, GBufferPrePass* gbuffer, bool isAnimationEnabled) override;;
    void Update(D3D* d3d, Heap* heap, TimeArgs timeArgs) override;
    void PostUpdate(D3D* d3d, const GreenHouseRenderInfo& renderInfo) override;
    void Render(D3D* d3d, ID3D12GraphicsCommandList* cmdList, const GreenHouseRenderInfo& renderInfo, D12Resource* RTV, D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle) override;
    void UnreserveData() override {};
    void GUI(GreenhouseConfig* config, bool& frameDirty) override;

    void MarkShadowMapMatricesDirty() { m_shadowMapManager.MarkMatricesDirty(); }

    size_t TotalCbvRequiredSize() override
    {
        return  Align(sizeof(CbvMatrices_VP), 256) +
                Align(sizeof(CbvForward), 256) +
                Align(sizeof(CbvShadowMapSample), 256) +
                Skybox::GetCbvRequiredSize() +
                ShadowMapManager::GetCbvRequiredSize();
    }

private:
    Pipeline m_pipelineGBuffer;
    RootSig m_rootSigGBuffer;
    DescriptorSet m_setGBuffer;
    RootConstants m_rootConstantsGBuffer;

    Pipeline m_pipelineLighting;
    RootSig m_rootSigLighting;
    DescriptorSet m_setLighting;

    Heap m_heapRTV;
    Heap m_heapDSV;
    uint32_t m_heapIdxMatIdx = 0;
    uint32_t m_heapIdxNormalsUv = 0;
    uint32_t m_heapIdxMv = 0;
    uint32_t m_heapIdxEmission = 0;
    uint32_t m_heapIdxDepth = 0;
    std::vector<DirectX::XMFLOAT4> m_rtvClearValues;

    DirectX::XMMATRIX m_prevV{};

    D12Resource m_gbufferTexMaterialIdx;
    D12Resource m_gbufferTexNormalsUv;
    D12Resource m_gbufferTexDepth;
    D12Resource m_gbufferTexMv;
    D12Resource m_gbufferTexEmission;

    D12Resource m_texPrimal;
    D12Resource m_texBloom;

    RtasBuilder m_rtasBuilder;
    RestirManager m_restirManager;

    Skybox m_skybox;
    std::wstring m_currentPanoFilepath;

    D12Resource m_brdfIntMap;

    ShadowMapManager m_shadowMapManager;
};


#endif //CHERRYSPROUT_FORWARD_H