#ifndef H_GREENHOUSE_H
#define H_GREENHOUSE_H

#include "Forward.h"
#include "PathTracer.h"
#include "HWI/DescriptorSet.h"
#include "HWI/Heap.h"
#include "HWI/RootSig.h"
#include "HWI/Shader.h"
#include "Scene/SceneManager.h"
#include "System/App.h"

enum class RenderBackend : uint32_t
{
    ePathTracer,
    eForward,
    eCount
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

private:
    SceneManager m_sceneManager;
    PathTracer m_pathTracer;
    Forward m_forward;
    RenderBackend m_renderBackend = RenderBackend::ePathTracer;

    Heap m_heap;
    UploadHeap m_uploadHeapCBV;
};

#endif