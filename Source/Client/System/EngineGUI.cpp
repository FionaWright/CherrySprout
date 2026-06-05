//
// Created by fionaw on 09/11/2025.
//

#include "System/pch.h"
#include "imgui.h"
#include "System/Config.h"
#include "System/Engine.h"
#include "System/Gui.h"
#include "System/Input.h"
#include "Utils/ConstantsCpp.h"

#ifdef _DEBUG
#   include "Debug/HotReloader.h"
#endif

void Engine::RenderGUI()
{
    const float startGuiX = Config::GetSystem().WindowAppGuiWidth + Config::GetSystem().RtvWidth;
    Gui::BeginWindow("Engine##xx", ImVec2(startGuiX,0), ImVec2(Config::GetSystem().WindowEngineGuiWidth, Config::GetSystem().RtvHeight));

    ImGui::SeparatorText("Stats##xx");
    ImGui::Indent(IM_GUI_INDENTATION);
    {
        const std::string fpsTxt = "FPS (Over 10ms): " + std::to_string(m_fps10ms);
        ImGui::Text("%s", fpsTxt.c_str());

        const std::string fpsTxt5 = "FPS (Over 50ms): " + std::to_string(m_fps50ms);
        ImGui::Text("%s", fpsTxt5.c_str());

        const std::string fpsTxt10 = "FPS (Over 100ms): " + std::to_string(m_fps100ms);
        ImGui::Text("%s", fpsTxt10.c_str());

        ImGui::Text("Frame Time (ms): %f", m_frameTime * 1000.0);

#ifdef _DEBUG
        static bool pauseFPSQueue = false;

        const bool canUpdateQueue = m_fpsGuiQueue.size() == 0 || m_fps10ms != m_fpsGuiQueue.at(m_fpsGuiQueue.size() - 1);
        if (canUpdateQueue && !pauseFPSQueue)
        {
            m_fpsGuiQueue.push_back(m_fps10ms);
            const float framesPer5Seconds = 5.0 * m_fps10ms;
            const int overflowFrames = m_fpsGuiQueue.size() - framesPer5Seconds;
            if (overflowFrames > 0)
                m_fpsGuiQueue.erase(m_fpsGuiQueue.begin(), m_fpsGuiQueue.begin() + overflowFrames);
        }

        if (ImGui::TreeNode("FPS Plot##xx"))
        {
            ImGui::Indent(IM_GUI_INDENTATION);

            ImGui::PlotLines("##xx", m_fpsGuiQueue.data(), m_fpsGuiQueue.size(), 0, nullptr, 0, 3.4028235E38F, ImVec2(0, 150));

            ImGui::Checkbox("Pause##xx", &pauseFPSQueue);

            ImGui::Unindent(IM_GUI_INDENTATION);
            ImGui::TreePop();
        }
#endif

        ImGui::Spacing();

        ImGui::Text("Mouse: (%f, %f)", Input::GetMousePos().x, Input::GetMousePos().y);
        ImGui::Text("Mouse Client: (%f, %f)", Input::GetMousePosClient().x, Input::GetMousePosClient().y);
    }
    ImGui::Unindent(IM_GUI_INDENTATION);

    ImGui::Spacing();

    ImGui::SeparatorText("Settings##xx");
    ImGui::Indent(IM_GUI_INDENTATION);
    {
        ImGui::Checkbox("VSync##xx", &Config::GetSystem().VSyncEnabled);
        ImGui::Checkbox("Sync GPU##xx", &Config::GetSystem().ForceSyncCpuGpu);
        ImGui::Checkbox("App Gui Enabled##xx", &Config::GetSystem().AppGuiEnabled);
        ImGui::InputFloat("Field of View##xx", &Config::GetRender().FoV);
        ImGui::InputFloat("Near Plane##xx", &Config::GetRender().NearPlane);
        ImGui::InputFloat("Far Plane##xx", &Config::GetRender().FarPlane);
    }
    ImGui::Unindent(IM_GUI_INDENTATION);

    ImGui::Spacing();

    ImGui::SeparatorText("Tools##xx");
    ImGui::Indent(IM_GUI_INDENTATION);
    {
#ifdef _DEBUG
        if (ImGui::Button("Reload All Pipelines"))
        {
            m_hotReloaderPendingAll = true;
        }
        if (ImGui::Button("Reload Graphics Pipelines"))
        {
            m_hotReloaderPendingGraphics = true;
        }
        if (ImGui::Button("Reload Compute Pipelines"))
        {
            m_hotReloaderPendingCompute = true;
        }
#endif
    }
    ImGui::Unindent(IM_GUI_INDENTATION);

    Gui::EndWindow();
}
