#ifndef H_GREENHOUSE_H
#define H_GREENHOUSE_H

#include "Forward.h"
#include "GreenhouseConfig.h"
#include "PathTracer.h"
#include "HWI/DescriptorSet.h"
#include "HWI/Heap.h"
#include "HWI/Shader.h"
#include "Render/CameraController.h"
#include "Scene/SceneManager.h"
#include "System/App.h"

class Greenhouse final : public App
{
public:
    [[nodiscard]] const char* GetName() const override { return "Greenhouse"; };

    void Init(D3D* d3d) override;
    void Update(D3D* d3d, TimeArgs timeArgs) override;
    void Render(D3D* d3d, ID3D12GraphicsCommandList* cmdList) override;
    void PostUpdate(D3D* d3d) override;
    void RenderGUI() override;

private:
    GreenhouseConfig m_config;
    float m_aspectRatio = 0.0f;
    XMMATRIX m_projectionMatrix{};

    CameraController m_cameraController;
    SceneManager m_sceneManager;

    PathTracer m_pathTracer;
    Forward m_forward;

    IRenderBackend* m_currRenderBackend = nullptr;

    Heap m_heap;
    UploadHeap m_uploadHeapCBV;
};

#endif