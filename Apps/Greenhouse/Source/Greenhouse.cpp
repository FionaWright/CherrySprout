#include "System/pch.h"
#include "Greenhouse.h"

#include "imgui.h"
#include "Debug/GPUEventScoped.h"
#include "HWI/D3D.h"
#include "PathTracing/CBVs.h"
#include "Scene/SceneManager.h"
#include "System/Gui.h"
#include "System/HighResolutionClock.h"
#include "Utils/Constants.h"
#include "Utils/Helper.h"
#include "Utils/D3DUtils.h"

//#define TEST_SCENE R"(C:\Users\fionawright\OneDrive\Documents\3D objects\USD\assets\full_assets\OpenChessSet\chess_set.usda)"
#define TEST_SCENE R"(C:\Users\fionawright\OneDrive\Documents\3D objects\USD\Cube\Cube.usda)"

void Greenhouse::Init(D3D* d3d)
{
    App::Init(d3d);

    m_sceneManager.LoadScene(TEST_SCENE);

    CURRENT ISSUE IS EACH GROUP OF 3 VERTICES IS THE SAME, NO DEDUPLICATION AS WELL. COMPARE SCENE LOADER

    std::cout << "Total PT      CBV Size: " << m_pathTracer.TotalCbvRequiredSize() << std::endl;
    std::cout << "Total Forward CBV Size: " << m_forward.TotalCbvRequiredSize() << std::endl;
    const size_t maxCbvRequiredSize = std::max(m_pathTracer.TotalCbvRequiredSize(), m_forward.TotalCbvRequiredSize());

    m_heap.Init("Test Heap", d3d->GetDevice(), 20000, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
    m_uploadHeapCBV.Init(d3d->GetDevice(), maxCbvRequiredSize + 256); // TODO: Test without extra

    m_cameraController.Init(XMFLOAT3(0, 0, 5), 0, PI);

    m_aspectRatio = static_cast<float>(Config::GetSystem().RtvWidth) / static_cast<float>(Config::GetSystem().RtvHeight);
    m_projectionMatrix = XMMatrixPerspectiveFovLH(XMConvertToRadians(Config::GetRender().FoV), m_aspectRatio, Config::GetRender().NearPlane, Config::GetRender().FarPlane);

    if (!d3d->GetRayTracingSupported())
    {
        if (m_config.RenderBackend == RenderBackendMode::ePathTracer)
            m_config.RenderBackend = RenderBackendMode::eForward;
    }

    m_currRenderBackend = m_config.RenderBackend == RenderBackendMode::eForward ? static_cast<IRenderBackend*>(&m_forward) : static_cast<IRenderBackend*>(&m_pathTracer);
}

void Greenhouse::Update(D3D* d3d, const TimeArgs timeArgs)
{
    m_cameraController.UpdateCamera(timeArgs.ElapsedTime_ms / 1000.0f);

    if (!m_currRenderBackend->IsInitialized())
    {
        m_currRenderBackend->Init(d3d, &m_heap, &m_uploadHeapCBV, &m_sceneManager.GetScene());
    }

    m_currRenderBackend->Update(d3d, timeArgs);
}

void Greenhouse::Render(D3D* d3d, ID3D12GraphicsCommandList* cmdList)
{
    // Upload Scene
    if (m_sceneManager.IsGpuDataDirty())
    {
        m_sceneManager.UploadScene(d3d, cmdList);
    }

    m_currRenderBackend->Render(d3d, cmdList, &m_heap, &m_sceneManager.GetScene(), m_cameraController.GetViewMatrix(), m_projectionMatrix);
}

void Greenhouse::PostUpdate(D3D* d3d)
{
}

void Greenhouse::RenderGUI()
{
    Gui::BeginWindow("Greenhouse", ImVec2(0, 0),
                     ImVec2(Config::GetSystem().WindowAppGuiWidth, Config::GetSystem().RtvHeight));

    bool switchedRenderBackends = false;

    static int e = static_cast<int>(m_config.RenderBackend);
    int c = 0;
    switchedRenderBackends |= ImGui::RadioButton("Path Tracer", &e, c++);
    switchedRenderBackends |= ImGui::RadioButton("Forward", &e, c++);
    m_config.RenderBackend = static_cast<RenderBackendMode>(e);

    if (switchedRenderBackends)
    {
        m_uploadHeapCBV.UnreserveData();
        m_sceneManager.UnreserveData();
        m_currRenderBackend->UnreserveData();
        m_currRenderBackend = m_config.RenderBackend == RenderBackendMode::eForward ? static_cast<IRenderBackend*>(&m_forward) : static_cast<IRenderBackend*>(&m_pathTracer);
    }

    Gui::EndWindow();
}
