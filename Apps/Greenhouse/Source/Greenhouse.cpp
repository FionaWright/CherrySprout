#include "System/pch.h"
#include "Greenhouse.h"

#include "imgui.h"
#include "Scenes.h"
#include "Debug/GPUEventScoped.h"
#include "HWI/D3D.h"
#include "PathTracing/CBVs.h"
#include "Scene/SceneManager.h"
#include "System/Gui.h"
#include "System/GuiUtils.h"
#include "System/HighResolutionClock.h"
#include "Utils/Constants.h"
#include "Utils/ConstantsCpp.h"
#include "Utils/Helper.h"
#include "Utils/D3DUtils.h"

void Greenhouse::Init(D3D* d3d)
{
    App::Init(d3d);

    m_currentSceneIdx = 0;
    Config::SetUIntFromArg(&m_currentSceneIdx, "--defaultSceneIndex");

    std::cout << "Total PT      CBV Size: " << m_pathTracer.TotalCbvRequiredSize() << std::endl;
    std::cout << "Total Forward CBV Size: " << m_forward.TotalCbvRequiredSize() << std::endl;
    const size_t maxCbvRequiredSize = std::max(m_pathTracer.TotalCbvRequiredSize(), m_forward.TotalCbvRequiredSize());

    m_heap.Init("Test Heap", d3d->GetDevice(), 20000, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
    m_uploadHeapCBV.Init(d3d->GetDevice(), maxCbvRequiredSize + 256); // TODO: Test without extra

    m_cameraController.Init(XMFLOAT3(0, 0, 5), 0, PI);

    m_aspectRatio = static_cast<float>(Config::GetSystem().RtvWidth) / static_cast<float>(Config::GetSystem().RtvHeight);
    m_projectionMatrix = XMMatrixPerspectiveFovLH(XMConvertToRadians(Config::GetRender().FoV), m_aspectRatio, Config::GetRender().NearPlane, Config::GetRender().FarPlane);
    m_invProjectionMatrix = XMMatrixInverse(nullptr, m_projectionMatrix);

    if (!d3d->GetRayTracingSupported())
    {
        if (m_config.RenderBackend == RenderBackendMode::ePathTracer)
            m_config.RenderBackend = RenderBackendMode::eForward;
    }

    m_currRenderBackend = m_config.RenderBackend == RenderBackendMode::eForward ? static_cast<IRenderBackend*>(&m_forward) : static_cast<IRenderBackend*>(&m_pathTracer);
    m_currRenderBackend->Init(d3d, &m_heap, &m_uploadHeapCBV);
}

void Greenhouse::Update(D3D* d3d, const TimeArgs timeArgs)
{
    if (m_renderBackendDirty)
    {
        m_uploadHeapCBV.FlushData();
        m_sceneManager.UnreserveData();
        m_currRenderBackend->UnreserveData();

        m_currRenderBackend = m_config.RenderBackend == RenderBackendMode::eForward ? static_cast<IRenderBackend*>(&m_forward) : static_cast<IRenderBackend*>(&m_pathTracer);

        if (!m_currRenderBackend->IsInitialized())
        {
            m_currRenderBackend->Init(d3d, &m_heap, &m_uploadHeapCBV);
        }

        m_currRenderBackend->SetSceneDataNotLoaded();
        m_renderBackendDirty = false;
    }

    if (m_sceneDirty)
    {
        m_sceneManager.UnreserveData();

        const SceneConfig& sceneConfig = s_sceneConfigs.at(m_currentSceneIdx);
        m_sceneManager.LoadScene(sceneConfig.Filepath.c_str());

        m_sceneDirty = false;
    }

    m_ptFrameDirty |= m_cameraController.UpdateCamera(timeArgs.ElapsedTime_ms / 1000.0f);
    if (m_ptFrameDirty)
    {
        m_pathTracer.Reset();
        m_ptFrameDirty = false;
    }

    if (m_ptPipelineDirty)
    {
        m_pathTracer.UpdatePipeline(d3d->GetDevice(), m_config.PathTracerConfig.FeatureFlags, m_config.PathTracerConfig.DebugFlags);
        m_ptPipelineDirty = false;
    }

    m_currRenderBackend->Update(d3d, timeArgs);
}

void Greenhouse::Render(D3D* d3d, ID3D12GraphicsCommandList* cmdList)
{
    // Upload Scene
    if (m_sceneManager.IsGpuDataDirty())
    {
        m_sceneManager.UploadScene(d3d, cmdList);
        m_currRenderBackend->SetSceneDataNotLoaded();
    }

    if (!m_currRenderBackend->IsSceneDataLoaded())
    {
        m_currRenderBackend->LoadSceneData(d3d, cmdList, &m_sceneManager.GetScene());
    }

    XMMATRIX V = m_cameraController.GetViewMatrix();
    XMMATRIX InvV = XMMatrixInverse(nullptr, V);

    GreenHouseRenderInfo renderInfo;
    renderInfo.Scene = &m_sceneManager.GetScene();
    renderInfo.Heap = &m_heap;
    renderInfo.Camera = &m_cameraController.GetCamera();
    renderInfo.V = &V;
    renderInfo.InvV = &InvV;
    renderInfo.P = &m_projectionMatrix;
    renderInfo.InvP = &m_invProjectionMatrix;
    renderInfo.BackendConfig = &m_config.RenderBackendConfig;
    renderInfo.PathTracerConfig = &m_config.PathTracerConfig;

    m_currRenderBackend->Render(d3d, cmdList, renderInfo);
}

void Greenhouse::PostUpdate(D3D* d3d)
{
}

void Greenhouse::RenderGUI()
{
    Gui::BeginWindow("Greenhouse", ImVec2(0, 0),
                     ImVec2(Config::GetSystem().WindowAppGuiWidth, Config::GetSystem().RtvHeight));

    ImGui::SeparatorText("Scene##xx");
    ImGui::Indent(IM_GUI_INDENTATION);
    {
        const char* curName = s_sceneConfigs.at(m_currentSceneIdx).Name.c_str();
        if (GuiUtils::BeginComboWithTooltip("Scene##xx", curName))
        {
            for (size_t i = 0; i < s_sceneConfigs.size(); i++)
            {
                const bool isSelected = m_currentSceneIdx == i;
                if (ImGui::Selectable(s_sceneConfigs.at(i).Name.c_str(), isSelected))
                {
                    m_currentSceneIdx = i;
                    m_sceneDirty = true;
                }

                if (isSelected)
                    ImGui::SetItemDefaultFocus();
            }

            ImGui::EndCombo();
        }

        m_sceneDirty |= ImGui::Button("Reload Scene##xx");
    }
    ImGui::Unindent(IM_GUI_INDENTATION);

    ImGui::SeparatorText("Render Backend##xx");
    ImGui::Indent(IM_GUI_INDENTATION);
    {
        static int e = static_cast<int>(m_config.RenderBackend);
        int c = 0;
        m_renderBackendDirty |= ImGui::RadioButton("Path Tracer", &e, c++);
        m_renderBackendDirty |= ImGui::RadioButton("Forward", &e, c++);
        m_config.RenderBackend = static_cast<RenderBackendMode>(e);
    }
    ImGui::Unindent(IM_GUI_INDENTATION);

    ImGui::SeparatorText("Settings##xx");
    ImGui::Indent(IM_GUI_INDENTATION);
    {
        ImGui::Text("Feature Flags:");
        ImGui::Indent(IM_GUI_INDENTATION);
        {
            for (int i = 0; i < FEATURE_COUNT; i++)
            {
                const auto flag = static_cast<PathTracerFeatureFlags>(1 << i);
                bool isEnabled = GetPathTracerFeatureFlag(m_config.PathTracerConfig.FeatureFlags, flag);
                m_ptPipelineDirty |= ImGui::Checkbox(s_featureFlagNames[i], &isEnabled);
                SetPathTracerFeatureFlag(m_config.PathTracerConfig.FeatureFlags, flag, isEnabled);
            }
        }
        ImGui::Unindent(IM_GUI_INDENTATION);

        ImGui::Text("Debug Flags:");
        ImGui::Indent(IM_GUI_INDENTATION);
        {
            for (int i = 0; i < DEBUG_COUNT; i++)
            {
                const auto flag = static_cast<PathTracerDebugFlags>(1 << i);
                bool isEnabled = GetPathTracerDebugFlag(m_config.PathTracerConfig.DebugFlags, flag);
                m_ptPipelineDirty |= ImGui::Checkbox(s_debugFlagNames[i], &isEnabled);
                SetPathTracerDebugFlag(m_config.PathTracerConfig.DebugFlags, flag, isEnabled);
            }
        }
        ImGui::Unindent(IM_GUI_INDENTATION);
    }
    ImGui::Unindent(IM_GUI_INDENTATION);

    m_ptFrameDirty |= m_sceneDirty | m_renderBackendDirty | m_ptPipelineDirty;

    Gui::EndWindow();
}
