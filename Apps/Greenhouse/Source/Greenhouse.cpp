#include "System/pch.h"
#include "Greenhouse.h"

#include "imgui.h"
#include "Scenes.h"
#include "Debug/GPUEventScoped.h"
#include "HWI/D3D.h"
#include "../../../Assets/Shaders/Utils/CBVs.h"
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
    Config::SetUIntFromArg(&m_currentSceneIdx, "--scene");
    Config::SetUIntFromArg(reinterpret_cast<uint32_t*>(&m_config.RenderBackend), "--backend");

    size_t maxCbvRequiredSize = 0;
    {
        std::cout << "Total PT      CBV Size: " << m_pathTracer.TotalCbvRequiredSize() << std::endl;
        std::cout << "Total Forward CBV Size: " << m_forward.TotalCbvRequiredSize() << std::endl;
        maxCbvRequiredSize += std::max(m_pathTracer.TotalCbvRequiredSize(), m_forward.TotalCbvRequiredSize());
        maxCbvRequiredSize += EnvironmentMap::GetCbvRequiredSize();
    }

    constexpr size_t numDescriptors = 20000; // TODO: Handle this properly
    m_heap.Init("Main Heap", d3d->GetDevice(), numDescriptors, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
    m_uploadHeapCBV.Init(d3d->GetDevice(), maxCbvRequiredSize + 256); // TODO: Test without extra

    m_cameraController.Init(XMFLOAT3(0, 0, 5), 0, PI);

    m_aspectRatio = static_cast<float>(Config::GetSystem().RtvWidth) / static_cast<float>(Config::GetSystem().RtvHeight);
    m_renderInfo.P = XMMatrixPerspectiveFovLH(XMConvertToRadians(Config::GetRender().FoV), m_aspectRatio, Config::GetRender().NearPlane, Config::GetRender().FarPlane);
    m_renderInfo.InvP = XMMatrixInverse(nullptr, m_renderInfo.P);
    m_renderInfo.V = m_cameraController.GetViewMatrix();
    m_renderInfo.InvV = XMMatrixInverse(nullptr, m_renderInfo.V);
    m_renderInfo.Heap = &m_heap;
    m_renderInfo.Camera = &m_cameraController.GetCamera();
    m_renderInfo.BackendConfig = &m_config.RenderBackendConfig;
    m_renderInfo.PathTracerConfig = &m_config.PathTracerConfig;
    m_renderInfo.EnvironmentMap = &m_envMap;

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

        m_renderBackendDirty = false;
    }

    if (m_sceneDirty)
    {
        m_sceneManager.UnreserveData();

        const SceneConfig& sceneConfig = s_sceneConfigs.at(m_currentSceneIdx);
        m_sceneManager.LoadScene(sceneConfig.Filepath.c_str(), sceneConfig.SceneScale);
        m_cameraController.GetCamera().SetPosition(sceneConfig.CameraPosition);
        m_cameraController.GetCamera().SetRotation(sceneConfig.CameraPitchYaw);

        m_renderInfo.Scene = &m_sceneManager.GetScene();

        m_sceneDirty = false;
    }

    // Upload Scene
    if (m_sceneManager.IsGpuDataDirty())
    {
        m_sceneManager.UploadScene(d3d);
        m_sceneManager.AddSceneTexturesToHeap(d3d, &m_heap);
    }

    if (m_envMapDirty)
    {
        m_envMap.Init(d3d, &m_heap, "autumn_field_puresky_4k.hdr", 0);
    }

    if (!m_currRenderBackend->IsSceneDataLoaded(m_sceneManager.GetScene().Filepath))
    {
        m_currRenderBackend->LoadSceneData(d3d, &m_sceneManager.GetScene(), &m_heap, &m_uploadHeapCBV, &m_envMap);
    }

    if (m_cameraController.UpdateCamera(timeArgs.ElapsedTime_ms / 1000.0f))
    {
        m_renderInfo.V = m_cameraController.GetViewMatrix();
        m_renderInfo.InvV = XMMatrixInverse(nullptr, m_renderInfo.V);
        m_ptFrameDirty = true;
    }

    if (m_ptFrameDirty)
    {
        m_pathTracer.Reset();
        m_ptFrameDirty = false;
    }

    if (m_ptPipelineDirty)
    {
        m_pathTracer.UpdatePipeline(d3d->GetDevice(),
            m_config.PathTracerConfig.FeatureFlags,
            m_config.PathTracerConfig.DebugFlags,
            m_config.PathTracerConfig.DebugOutputIdx,
            m_config.PathTracerConfig.BxdfMode);
        m_ptPipelineDirty = false;
    }

    m_currRenderBackend->Update(d3d, timeArgs);
}

void Greenhouse::Render(D3D* d3d, ID3D12GraphicsCommandList* cmdList)
{
    m_renderInfo.EnvMapDirty = m_envMapDirty;

    m_currRenderBackend->Render(d3d, cmdList, m_renderInfo);
}

void Greenhouse::PostUpdate(D3D* d3d)
{
    m_currRenderBackend->PostUpdate(d3d, m_renderInfo);

    m_envMapDirty = false;
}

bool Greenhouse::pathTracingFeatureEnabled(const PathTracerFeatureFlags flag) const
{
    if (m_config.RenderBackend != RenderBackendMode::ePathTracer)
        return false;

    return GetPathTracerFeatureFlag(m_config.PathTracerConfig.FeatureFlags, flag);
}

void Greenhouse::RenderGUI()
{
    Gui::BeginWindow("Greenhouse", ImVec2(0, 0),
                     ImVec2(Config::GetSystem().WindowAppGuiWidth, Config::GetSystem().RtvHeight));

    static bool s_hideGUI = false;
    ImGui::Checkbox("Hide GUI", &s_hideGUI);
    if (s_hideGUI)
    {
        Gui::EndWindow();
        return;
    }

    ImGui::SeparatorText("Scene");
    ImGui::Indent(IM_GUI_INDENTATION);
    {
        const char* curName = s_sceneConfigs.at(m_currentSceneIdx).Name.c_str();
        if (GuiUtils::BeginComboWithTooltip("Scene", curName))
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

        m_sceneDirty |= ImGui::Button("Reload Scene");
    }
    ImGui::Unindent(IM_GUI_INDENTATION);

    ImGui::SeparatorText("Render Backend");
    ImGui::Indent(IM_GUI_INDENTATION);
    {
        static int e = static_cast<int>(m_config.RenderBackend);
        int c = 0;
        m_renderBackendDirty |= ImGui::RadioButton("Path Tracer", &e, c++);
        m_renderBackendDirty |= ImGui::RadioButton("Forward", &e, c++);
        m_config.RenderBackend = static_cast<RenderBackendMode>(e);
    }
    ImGui::Unindent(IM_GUI_INDENTATION);

    ImGui::SeparatorText("Info");
    ImGui::Indent(IM_GUI_INDENTATION);
    {
        ImGui::Text("Frames : %i", m_pathTracer.GetCurrentFrameIdx());
        ImGui::Text("Samples: %i", m_pathTracer.GetCurrentFrameIdx() * m_config.PathTracerConfig.SPP);
    }
    ImGui::Unindent(IM_GUI_INDENTATION);

    ImGui::SeparatorText("Settings##PT");
    ImGui::Indent(IM_GUI_INDENTATION);
    {

        m_ptFrameDirty |= GuiUtils::FwInputUInt("SPP", &m_config.PathTracerConfig.SPP);
        m_ptFrameDirty |= GuiUtils::FwInputUInt("Max Ray Depth", &m_config.PathTracerConfig.MaxRayDepth);
        m_ptFrameDirty |= GuiUtils::FwInputUInt("Max Frames", &m_config.PathTracerConfig.MaxFrameNumber);
        m_ptFrameDirty |= GuiUtils::FwInputUInt("RR Min Bounces", &m_config.PathTracerConfig.RussianRouletteMinBounces);
        m_ptFrameDirty |= GuiUtils::FwInputFloat("Firefly Threshold", &m_config.PathTracerConfig.FireFlyThreshold);

        ImGui::Spacing();

        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0, 0));

        // BxDF Mode
        {
            ImGui::Text("BxDF:");
            ImGui::Indent(IM_GUI_INDENTATION);
            {
                static int e = static_cast<int>(m_config.PathTracerConfig.BxdfMode);
                int c = 0;
                for (int i = 0; i < static_cast<int>(BxdfMode::eCount); i++)
                {
                    m_ptPipelineDirty |= ImGui::RadioButton(s_bxdfNames[i], &e, c++);
                }
                m_config.PathTracerConfig.BxdfMode = static_cast<BxdfMode>(e);
            }
            ImGui::Unindent(IM_GUI_INDENTATION);
        }

        ImGui::Text("Feature Flags:");
        ImGui::Indent(IM_GUI_INDENTATION);
        if (ImGui::BeginTable("Feature Flags", 2))
        {
            for (int i = 0; i < FEATURE_COUNT; i++)
            {
                ImGui::TableNextColumn();

                const auto flag = static_cast<PathTracerFeatureFlags>(1 << i);
                bool isEnabled = GetPathTracerFeatureFlag(m_config.PathTracerConfig.FeatureFlags, flag);
                m_ptPipelineDirty |= ImGui::Checkbox(s_featureFlagNames[i], &isEnabled);
                SetPathTracerFeatureFlag(m_config.PathTracerConfig.FeatureFlags, flag, isEnabled);

                ImGui::SetItemTooltip("%s", s_featureFlagNames[i]);
            }

            ImGui::EndTable();
        }
        ImGui::Unindent(IM_GUI_INDENTATION);
        ImGui::Spacing();

        if (pathTracingFeatureEnabled(eFeature_DirectionalLight))
        {
            ImGui::Text("Directional Light:");
            ImGui::Indent(IM_GUI_INDENTATION);
            {
                m_ptFrameDirty |= GuiUtils::FwInputFloat3("Dir Light Dir", (float*)&m_config.RenderBackendConfig.DirLightDirection);
                m_ptFrameDirty |= GuiUtils::FwColorEdit3("Dir Light Color", (float*)&m_config.RenderBackendConfig.DirLightColor);
                m_ptFrameDirty |= GuiUtils::FwInputFloat("Dir Light Intensity", &m_config.RenderBackendConfig.DirLightIntensity);
                m_ptFrameDirty |= GuiUtils::FwInputFloat("Dir Light Radius", &m_config.RenderBackendConfig.DirLightCosAngularRadius);
            }
            ImGui::Unindent(IM_GUI_INDENTATION);
            ImGui::Spacing();
        }

        if (pathTracingFeatureEnabled(eFeature_DepthOfField))
        {
            ImGui::Text("Depth of Field:");
            ImGui::Indent(IM_GUI_INDENTATION);
            {
                m_ptFrameDirty |= GuiUtils::FwDragFloat("Focal Distance", &m_config.PathTracerConfig.DofFocalDist, 0.05f);
                m_ptFrameDirty |= GuiUtils::FwInputFloat("Lens Radius", &m_config.PathTracerConfig.DofLensRadius);
            }
            ImGui::Unindent(IM_GUI_INDENTATION);
            ImGui::Spacing();
        }

        ImGui::Text("Debug Flags:");
        ImGui::Indent(IM_GUI_INDENTATION);
        if (ImGui::BeginTable("Debug Flags", 2))
        {
            for (int i = 0; i < DEBUG_COUNT; i++)
            {
                ImGui::TableNextColumn();

                const auto flag = static_cast<PathTracerDebugFlags>(1 << i);
                bool isEnabled = GetPathTracerDebugFlag(m_config.PathTracerConfig.DebugFlags, flag);
                m_ptPipelineDirty |= ImGui::Checkbox(s_debugFlagNames[i], &isEnabled);
                SetPathTracerDebugFlag(m_config.PathTracerConfig.DebugFlags, flag, isEnabled);

                ImGui::SetItemTooltip("%s", s_debugFlagNames[i]);
            }
            ImGui::EndTable();
        }
        ImGui::Unindent(IM_GUI_INDENTATION);
        ImGui::Spacing();

#ifdef _DEBUG
        if (GetPathTracerDebugFlag(m_config.PathTracerConfig.DebugFlags, eDebug_Asserts))
        {
            static_cast<PathTracer*>(m_currRenderBackend)->RenderGUI_ErrorInfo();
        }
#endif

        if (!GetPathTracerDebugFlag(m_config.PathTracerConfig.DebugFlags, eDebug_OutputColor))
            m_config.PathTracerConfig.DebugOutputIdx = DebugOutputIndex::eDebugOutput_Disabled;

        if (GetPathTracerDebugFlag(m_config.PathTracerConfig.DebugFlags, eDebug_OutputColor))
        {
            ImGui::Text("Debug Outputs:");
            ImGui::Indent(IM_GUI_INDENTATION);
            if (ImGui::BeginTable("Debug Outputs", 2))
            {
                static int e = static_cast<int>(m_config.PathTracerConfig.DebugOutputIdx);
                int c = 1;
                for (int i = 1; i < static_cast<int>(DebugOutputIndex::eCount); i++)
                {
                    ImGui::TableNextColumn();
                    m_ptPipelineDirty |= ImGui::RadioButton(s_debugOutputIdxNames[i], &e, c++);
                    ImGui::SetItemTooltip("%s", s_debugOutputIdxNames[i]);
                }
                m_config.PathTracerConfig.DebugOutputIdx = static_cast<DebugOutputIndex>(e);
            }
            ImGui::Unindent(IM_GUI_INDENTATION);
            ImGui::EndTable();
        }

        ImGui::PopStyleVar();
    }
    ImGui::Unindent(IM_GUI_INDENTATION);

    m_ptFrameDirty |= m_sceneDirty | m_renderBackendDirty | m_ptPipelineDirty;

    Gui::EndWindow();
}
