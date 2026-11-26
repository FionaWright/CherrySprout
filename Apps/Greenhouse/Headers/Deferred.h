//
// Created by fiona on 01/06/2026.
//

#ifndef CHERRYSPROUT_DEFERRED_H
#define CHERRYSPROUT_DEFERRED_H

#include "GBufferPrePass.h"
#include "IRenderBackend.h"
#include "HWI/DescriptorSet.h"
#include "HWI/RootSig.h"
#include "HWI/Pipeline.h"
#include "HWI/RtasBuilder.h"
#include "Render/RestirManager.h"
#include "Render/ShadowMapManager.h"
#include "Render/Skybox.h"
#include "Utils/CBVs.h"

struct Scene;
struct TimeArgs;

#define MAX_BLOOM_DEPTH 10

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
        return  Align(sizeof(CbvForward), 256) +
                Align(sizeof(CbvShadowMapSample), 256) +
                Align(sizeof(CbvPostProcessing), 256) +
                Skybox::GetCbvRequiredSize() +
                GBufferPrePass::GetCbvRequiredSize() +
                ShadowMapManager::GetCbvRequiredSize();
    }

private:
    Pipeline m_pipelineLighting;
    RootSig m_rootSigLighting;
    DescriptorSet m_setLighting;

    Pipeline m_pipelineBloomH;
    Pipeline m_pipelineBloomV;
    Pipeline m_pipelineBloomExtract;
    Pipeline m_pipelineBloomDownsample;
    Pipeline m_pipelineBloomUpsample;
    RootSig m_rootSigBloom;
    DescriptorSet m_setBloomExtract;
    DescriptorSet m_setBloomH;
    DescriptorSet m_setBloomV;
    DescriptorSet m_setBloomDown[MAX_BLOOM_DEPTH];
    DescriptorSet m_setBloomUp[MAX_BLOOM_DEPTH];
    RootConstants m_rootConstantsBloom;

    Pipeline m_pipelinePostProcess;
    RootSig m_rootSigPostProcess;
    DescriptorSet m_setPostProcess;

    D12Resource m_texPrimal;
    D12Resource m_texPing;
    D12Resource m_texPong;
    D12Resource m_texBloom[MAX_BLOOM_DEPTH];
    D12Resource m_texOutputRgba8Unorm;

    RtasBuilder m_rtasBuilder;
    RestirManager m_restirManager;
    GBufferPrePass m_gbufferPrePass;

    Skybox m_skybox;
    std::wstring m_currentPanoFilepath;

    D12Resource m_brdfIntMap;

    ShadowMapManager m_shadowMapManager;
};


#endif //CHERRYSPROUT_FORWARD_H