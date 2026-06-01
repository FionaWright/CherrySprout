#include "System/pch.h"
#include "Greenhouse.h"

#include "imgui.h"
#include "Debug/GPUEventScoped.h"
#include "HWI/D3D.h"
#include "PathTracing/CBVs.h"
#include "Scene/SceneManager.h"
#include "System/Gui.h"
#include "System/HighResolutionClock.h"
#include "Utils/Helper.h"
#include "Utils/D3DUtils.h"

void Greenhouse::Init(D3D* d3d)
{
    App::Init(d3d);

    m_sceneManager.LoadScene(
        R"(C:\Users\fionawright\OneDrive\Documents\3D objects\USD\assets\full_assets\OpenChessSet\chess_set.usda)");

    const size_t pathTracerCbvRequiredSize = PathTracer::TotalCbvRequiredSize();

    m_heap.Init("Test Heap", d3d->GetDevice(), 20000, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
    m_uploadHeapCBV.Init(d3d->GetDevice(), pathTracerCbvRequiredSize + 256);

    m_pathTracer.Init(d3d, &m_heap, &m_uploadHeapCBV, &m_sceneManager.GetScene());
}

void Greenhouse::Update(D3D* d3d, const TimeArgs timeArgs)
{
    m_pathTracer.Update(d3d, timeArgs);
}

void Greenhouse::Render(D3D* d3d, ID3D12GraphicsCommandList* cmdList)
{
    // Upload Scene
    if (m_sceneManager.IsGpuDataDirty())
    {
        m_sceneManager.UploadScene(d3d, cmdList);
    }

    m_pathTracer.Render(d3d, cmdList, &m_heap, &m_sceneManager.GetScene());

    // Copy to RTV
    {
        GPU_SCOPE(cmdList, "Copy PT Output to RTV");

        D12Resource* ptOut = m_pathTracer.GetTexOutput();
        D12Resource* rtv = d3d->GetRtv();

        ptOut->Transition(cmdList, D3D12_RESOURCE_STATE_COPY_SOURCE);
        rtv->Transition(cmdList, D3D12_RESOURCE_STATE_COPY_DEST);

        rtv->CopyTextureInto(cmdList, ptOut->GetResource(), Config::GetSystem().WindowAppGuiWidth, 0, 0);
    }
}

void Greenhouse::PostUpdate(D3D* d3d)
{
}

void Greenhouse::RenderGUI()
{
    Gui::BeginWindow("Greenhouse", ImVec2(0, 0),
                     ImVec2(Config::GetSystem().WindowAppGuiWidth, Config::GetSystem().RtvHeight));

    ImGui::Text("Test");

    Gui::EndWindow();
}
