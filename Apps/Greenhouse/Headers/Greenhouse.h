#ifndef H_GREENHOUSE_H
#define H_GREENHOUSE_H

#include "Forward.h"
#include "GreenhouseConfig.h"
#include "PathTracer.h"
#include "HWI/DescriptorSet.h"
#include "HWI/Heap.h"
#include "Render/CameraController.h"
#include "Render/EnvironmentMap.h"
#include "Render/LightImportanceSampler.h"
#include "Scene/SceneManager.h"
#include "System/App.h"

#if CHERRY_DEBUG_FEATURES_ENABLED
#   include "Render/GizmoManager.h"
#endif

struct GreenHouseRenderInfo
{
    Camera* Camera;
    XMMATRIX V;
    XMMATRIX InvV;
    XMMATRIX P;
    XMMATRIX InvP;
    Scene* Scene;
    Heap* Heap;

    EnvironmentMap* EnvironmentMap;
    LightImportanceSampler* LightImportanceSampler;

    bool EnvMapDirty = false;

    RenderBackendConfig* BackendConfig;
    PathTracerConfig* PathTracerConfig;
};

class Greenhouse final : public App
{
public:
    [[nodiscard]] const char* GetName() const override { return "Greenhouse"; };

    void Init(D3D* d3d) override;
    void Update(D3D* d3d, TimeArgs timeArgs) override;
    void Render(D3D* d3d, ID3D12GraphicsCommandList* cmdList) override;
    void PostUpdate(D3D* d3d) override;
    void RenderGUI() override;
    void OnResize(uint32_t width, uint32_t height) override;

    [[nodiscard]] bool GBufferRequired() const;

private:
    void renderGuiCore();
    void renderGuiSceneData();

    GreenhouseConfig m_config{};
    float m_aspectRatio = 0.0f;
    uint32_t m_currentSceneIdx = 0;

    bool m_sceneDirty = true;
    bool m_envMapDirty = true;
    bool m_renderBackendDirty = false;
    bool m_ptFrameDirty = false;
    bool m_ptPipelineDirty = false;
    bool m_lsdDirty = false;
    bool m_renderBackendSceneDataDirty = false;

    CameraController m_cameraController;

#if CHERRY_DEBUG_FEATURES_ENABLED
    GizmoManager m_gizmoManager;
    bool m_gizmosEnabled = true;
    bool m_gizmosSceneLoaded = false;
#endif

    D12Resource m_frameBuffer;
    Heap m_heapRTV;
    uint32_t m_heapIdxFrameBuffer = 0;

    SceneManager m_sceneManager;
    EnvironmentMap m_envMap;
    LightImportanceSampler m_lightImportanceSampler;
    GBufferPrePass m_gbufferPrePass;

    PathTracer m_pathTracer = {};
    Forward m_forward;

    IRenderBackend* m_currRenderBackend = nullptr;
    GreenHouseRenderInfo m_renderInfo = {};

    Heap m_heap;
    UploadHeap m_uploadHeapCBV;

#if !NDEBUG
    std::string m_scheduledSnapshotPT = "";
    bool m_isSnapshotPtLDR = false;

    int m_scheduledTransientRenderFrameIdx = -1;
    uint32_t m_scheduledTransientRenderSampleIdx = 0;
#endif
};

#endif