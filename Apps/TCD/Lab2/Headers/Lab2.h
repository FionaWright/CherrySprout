//
// Created by fiona on 01/10/2026.
//

#ifndef CHERRYSPROUT_LAB2_H
#define CHERRYSPROUT_LAB2_H

#include "HWI/DescriptorSet.h"
#include "HWI/Pipeline.h"
#include "HWI/RootSig.h"
#include "HWI/RtasBuilder.h"
#include "Render/CameraController.h"
#include "Render/EnvironmentMap.h"
#include "Render/Skybox.h"
#include "Scene/Scene.h"
#include "Scene/SceneManager.h"
#include "System/App.h"

class Lab2 final : public App
{
public:
    void Init(D3D* d3d) override;
    void Update(D3D* d3d, TimeArgs timeArgs) override;
    void Render(D3D* d3d, ID3D12GraphicsCommandList* cmdList) override;
    void PostUpdate(D3D* d3d) override;
    void RenderGUI() override;
    void OnResize(uint32_t width, uint32_t height) override;

    [[nodiscard]] const char* GetName() const override { return "TCD Lab2"; }

private:
    float m_aspectRatio = 0.0f;

    Pipeline m_pipeline;
    RootSig m_rootSig;
    DescriptorSet m_descriptorSet;
    RootConstants m_rootConstants;

    RtasBuilder m_rtasBuilder;

    SceneManager m_sceneManager;

    bool m_cameraDirty = true;
    bool m_envMapDirty = true;

    EnvironmentMap m_envMap;
    Skybox m_skybox;
    CameraController m_cameraController;

    Heap m_heap;
    UploadHeap m_uploadHeapCBV;

    std::vector<float> m_scales;
    XMFLOAT3 m_dirLightDir = XMFLOAT3(-1,-1,-0.5);

    DirectX::XMMATRIX m_P = {};
    DirectX::XMMATRIX m_V = {};
    DirectX::XMMATRIX m_InvP = {};
    DirectX::XMMATRIX m_InvV = {};
};


#endif //CHERRYSPROUT_LAB2_H