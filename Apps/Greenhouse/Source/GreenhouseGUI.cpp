#include "System/pch.h"
#include "Greenhouse.h"

#include "imgui.h"
#include "Scenes.h"
#include "System/Gui.h"
#include "System/GuiUtils.h"
#include "Utils/ConstantsCpp.h"
#include "Utils/D3DUtils.h"
#include "Utils/Helper.h"

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

#if CHERRY_DEBUG_FEATURES_ENABLED
    m_pathTracer.RenderGUI_DebugInfo(m_config.PathTracerConfig);
#endif

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
                for (auto & s_bxdfName : s_bxdfNames)
                {
                    m_ptPipelineDirty |= ImGui::RadioButton(s_bxdfName, &e, c++);
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

#if CHERRY_DEBUG_FEATURES_ENABLED
        if (pathTracingFeatureEnabled(eFeature_NEE) && ImGui::CollapsingHeader("CDF"))
        {
            ImGui::Indent(IM_GUI_INDENTATION);
            {
                ImGui::Text("Env Map Luminance: %f", m_lightImportanceSampler.GetTotalEnvMapLuminance());
                ImGui::Text("Punctual Weight  : %f", m_lightImportanceSampler.GetPunctualWeight());

                const auto& cpuLightCdf = m_lightImportanceSampler.GetCpuLightsCdf();
                for (int i = 0; i < cpuLightCdf.size(); ++i)
                {
                    if (i == 0)
                        ImGui::Text("%i (EnvMap)   : PMF=%.3f, CDF=%.3f", i, cpuLightCdf[i].PMF, cpuLightCdf[i].CDF);
                    else
                        ImGui::Text("%i (Punctual) : PMF=%.3f, CDF=%.3f", i, cpuLightCdf[i].PMF, cpuLightCdf[i].CDF);
                }
            }
            ImGui::Unindent(IM_GUI_INDENTATION);
            ImGui::Spacing();
        }
#endif

        m_lightCdfsDirty |= ImGui::Button("Reload Light CDFs");

#if CHERRY_DEBUG_FEATURES_ENABLED
        ImGui::Text("Debug Flags:");
        ImGui::Indent(IM_GUI_INDENTATION);
        if (ImGui::BeginTable("Debug Flags", 2))
        {
            for (int i = 0; i < DEBUG_COUNT; i++)
            {
                ImGui::TableNextColumn();

                const auto flag = static_cast<PathTracerDebugFlags>(1 << i);
                bool isEnabled = GetPathTracerDebugFlag(m_config.PathTracerConfig.DebugInfo.Flags, flag);
                m_ptPipelineDirty |= ImGui::Checkbox(s_debugFlagNames[i], &isEnabled);
                SetPathTracerDebugFlag(m_config.PathTracerConfig.DebugInfo.Flags, flag, isEnabled);
                ImGui::IsItemDeactivatedAfterEdit();

                ImGui::SetItemTooltip("%s", s_debugFlagNames[i]);
            }
            ImGui::EndTable();
        }
        ImGui::Unindent(IM_GUI_INDENTATION);
        ImGui::Spacing();

        if (!GetPathTracerDebugFlag(m_config.PathTracerConfig.DebugInfo.Flags, eDebug_OutputColor))
        {
            m_config.PathTracerConfig.DebugInfo.OutputColorIdx = DebugOutputIndex::eDebugOutput_Disabled;
            m_config.PathTracerConfig.DebugInfo.OutputColorRemap = DebugOutputColorRemap::eNone;
        }

        if (GetPathTracerDebugFlag(m_config.PathTracerConfig.DebugInfo.Flags, eDebug_OutputColor))
        {
            ImGui::Text("Debug Outputs:");
            ImGui::Indent(IM_GUI_INDENTATION);

            m_ptFrameDirty |= ImGui::InputInt("Chosen Ray Depth", &m_config.PathTracerConfig.DebugInfo.ChosenRayDepth);
            ImGui::IsItemDeactivatedAfterEdit();
            m_config.PathTracerConfig.DebugInfo.ChosenRayDepth = max(-1, m_config.PathTracerConfig.DebugInfo.ChosenRayDepth);

            ImGui::Text("Remap:");
            ImGui::Indent(IM_GUI_INDENTATION);
            if (ImGui::BeginTable("Debug Output Color Remaps", 3))
            {
                const auto remapAsUint = static_cast<uint32_t>(m_config.PathTracerConfig.DebugInfo.OutputColorRemap);
                for (int i = 0; i < _countof(s_debugOutputColorRemapNames); i++)
                {
                    ImGui::TableNextColumn();

                    const uint32_t flag = 1 << i;
                    bool isEnabled = remapAsUint & flag;
                    const bool prevIsEnabled = isEnabled;
                    const bool clicked = ImGui::Checkbox(s_debugOutputColorRemapNames[i], &isEnabled);
                    m_ptFrameDirty |= clicked;
                    ImGui::IsItemDeactivatedAfterEdit();

                    if (clicked && isEnabled)
                        m_config.PathTracerConfig.DebugInfo.OutputColorRemap = static_cast<DebugOutputColorRemap>(remapAsUint | flag);
                    else if (clicked && prevIsEnabled)
                        m_config.PathTracerConfig.DebugInfo.OutputColorRemap = static_cast<DebugOutputColorRemap>(remapAsUint ^ flag);

                    ImGui::SetItemTooltip("%s", s_debugOutputColorRemapNames[i]);
                }
                ImGui::EndTable();
            }
            ImGui::Unindent(IM_GUI_INDENTATION);

            if (ImGui::BeginTable("Debug Outputs", 2))
            {
                static int e = static_cast<int>(m_config.PathTracerConfig.DebugInfo.OutputColorIdx);
                int c = 1;
                for (int i = 1; i < static_cast<int>(DebugOutputIndex::eCount); i++)
                {
                    ImGui::TableNextColumn();
                    m_ptFrameDirty |= ImGui::RadioButton(s_debugOutputIdxNames[i], &e, c++);
                    ImGui::IsItemDeactivatedAfterEdit();
                    ImGui::SetItemTooltip("%s", s_debugOutputIdxNames[i]);
                }
                m_config.PathTracerConfig.DebugInfo.OutputColorIdx = static_cast<DebugOutputIndex>(e);
            }
            ImGui::Unindent(IM_GUI_INDENTATION);
            ImGui::EndTable();
        }
#endif

        ImGui::PopStyleVar();
    }
    ImGui::Unindent(IM_GUI_INDENTATION);

    ImGui::SeparatorText("Tools:");
    ImGui::Indent(IM_GUI_INDENTATION);
    {
#if CHERRY_DEBUG_FEATURES_ENABLED
        static char buff[256];
        ImGui::InputText("Snapshot Path", buff, 256);

        if (ImGui::Button("Take Snapshot (PT)"))
        {
            if (buff[0] == '\0')
                m_scheduledSnapshotPT = std::string(BUILD_DIR) + "/Snapshots/Default_PT";
            else
                m_scheduledSnapshotPT = std::string(BUILD_DIR) + "/Snapshots/" + buff;
        }

        if (ImGui::Button("Print Camera Location"))
        {
            const float posX = m_cameraController.GetCamera().GetPosition().x;
            const float posY = m_cameraController.GetCamera().GetPosition().y;
            const float posZ = m_cameraController.GetCamera().GetPosition().z;
            CherryPrint(".CameraPosition = {" << std::to_string(posX) << ", " << std::to_string(posY) << ", " << std::to_string(posZ) << "},");
            const float pitch = m_cameraController.GetCamera().GetPitch();
            const float yaw = m_cameraController.GetCamera().GetYaw();
            CherryPrint(".CameraPitchYaw = {" << std::to_string(pitch) << ", " << std::to_string(yaw) << "},");
        }
#endif
    }
    ImGui::Unindent(IM_GUI_INDENTATION);

    m_ptFrameDirty |= m_sceneDirty | m_renderBackendDirty | m_ptPipelineDirty;

    Gui::EndWindow();
}
