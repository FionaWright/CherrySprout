#ifndef H_GREENHOUSE_H
#define H_GREENHOUSE_H

#include "HWI/DescriptorSet.h"
#include "HWI/Heap.h"
#include "HWI/RootSig.h"
#include "HWI/Shader.h"
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
    SceneManager m_sceneManager;

    Shader m_shaderPT;
    RootSig m_rootSigPT;
    DescriptorSet m_descriptorSet;
    D12Resource m_output, m_accum;

    Heap m_heap;
    UploadHeap m_uploadHeapCBV;
};

#endif