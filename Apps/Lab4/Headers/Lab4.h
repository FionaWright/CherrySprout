#ifndef CHERRYSPROUT_LAB3_H
#define CHERRYSPROUT_LAB3_H

#include "HWI/DescriptorSet.h"
#include "HWI/Pipeline.h"
#include "HWI/RootSig.h"
#include "HWI/RtasBuilder.h"
#include "Render/CameraController.h"
#include "Render/EnvironmentMap.h"
#include "Render/ShadowMapManager.h"
#include "Render/Skybox.h"
#include "Scene/AnimationManager.h"
#include "Scene/Scene.h"
#include "Scene/SceneManager.h"
#include "System/App.h"

class Lab4 final : public App
{
public:
    void Init(D3D* d3d) override;
    void Update(D3D* d3d, TimeArgs timeArgs) override;
    void Render(D3D* d3d, ID3D12GraphicsCommandList* cmdList) override;
    void PostUpdate(D3D* d3d) override;
    void RenderGUI() override;
    void OnResize(uint32_t width, uint32_t height) override;

    [[nodiscard]] const char* GetName() const override { return "TCD Lab4"; }

private:
    float m_aspectRatio = 0.0f;

    Pipeline m_pipeline;
    RootSig m_rootSig;
    DescriptorSet m_set;
    RootConstants m_rootConstants;

    RtasBuilder m_rtasBuilder;
    CameraController m_cameraController;
    ShadowMapManager m_shadowMapManager;
    SceneManager m_sceneManager;
    AnimationManager m_animationManager;
    AnimationType m_animType = AnimationType::eLBS;

    bool m_cameraDirty = true;
    bool m_skyboxEnabled = true;
    bool m_punctualsEnabled = true;
    bool m_ambientEnabled = true;
    bool m_shadowMapsEnabled = true;
    bool m_dirLightEnabled = true;

    uint32_t m_smPcfCount = 16;
    float m_smWidth = 2.0f;
    float m_smHeight = 2.0f;
    float m_smNear = 0.01f;
    float m_smFar = 100.0f;
    float m_smDirDist = 50.0f;
    float m_smPcfRadius = 1.5f;
    float m_smBiasDepthDistant = 0.000001f;
    float m_smBiasDepthPoint = 0.001f;
    float m_smBiasNormalDistant = 0.01f;
    float m_smBiasNormalPoint = 0.001f;

    EnvironmentMap m_envMap;
    Skybox m_skybox;
    D12Resource m_brdfIntMap;

    Heap m_heap;
    UploadHeap m_uploadHeapCBV;

    std::vector<XMFLOAT3> m_positions;
    std::vector<float> m_scales;
    std::vector<int> m_renderOrder;
    XMFLOAT3 m_dirLightDir = XMFLOAT3(-1,-1,-0.5);

    XMMATRIX m_P = {};
    XMMATRIX m_V = {};
    XMMATRIX m_InvP = {};
    XMMATRIX m_InvV = {};

    uint32_t m_frameIdx = 0;
};


#endif