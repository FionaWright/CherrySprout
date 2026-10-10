#include "System/pch.h"
#include "DebugManager.h"

#include "Greenhouse.h"
#include "../../../../Assets/Shaders/PathTracing/FeatFlags/FeatFlags.hpp"
#include "Utils/ConstantsCpp.h"
#include "imgui.h"
#include "GreenhouseConfig.h"
#include "System/GuiUtils.h"
#include "System/Input.h"

void DebugManager::GUI(PathTracerConfig& config, bool& ptFrameDirty)
{
    if (ImGui::CollapsingHeader("RMSE Tool"))
    {
        ImGui::Indent(IM_GUI_INDENTATION);

        ImGui::TextUnformatted("Selected Slot:");
        int e = m_rmseTool.GetSelectedSlot();
        int idx = 0;
        ImGui::Indent(IM_GUI_INDENTATION);
        ImGui::RadioButton(m_rmseTool.GetSlotName(0).c_str(), &e, idx++);
        ImGui::RadioButton(m_rmseTool.GetSlotName(1).c_str(), &e, idx++);
        ImGui::Unindent(IM_GUI_INDENTATION);
        m_rmseTool.SetSelectedSlot(static_cast<uint32_t>(e));

        static char buff[256];
        ImGui::InputText("Path", buff, 256);

        const bool isInActiveState = m_rmseTool.GetCurrentState() != RmseToolState::eIdle;
        if (isInActiveState)
            ImGui::BeginDisabled();
        {
            if (ImGui::Button("Store PT Output"))
            {
                m_rmseTool.TriggerStoreNextOutput();
            }

            if (ImGui::Button("Save To File"))
            {
                const std::string fullPath = std::string(BUILD_DIR) + "/Snapshots/Golden/" + buff + ".hdr";
                m_rmseTool.TriggerSaveToFile(fullPath);
            }

            if (ImGui::Button("Load From File"))
            {
                const std::string fullPath = std::string(BUILD_DIR) + "/Snapshots/Golden/" + buff + ".hdr";
                m_rmseTool.TriggerLoadFromFile(fullPath);
            }

            if (ImGui::Button("Compute RMSE"))
            {
                m_rmseTool.TriggerComputeSingleRMSE();
            }
        }
        if (isInActiveState)
            ImGui::EndDisabled();

        ImGui::Text("%s", (std::string("RMSE: ") + std::to_string(m_rmseTool.GetComputedRMSE())).c_str());

        ImGui::Spacing();

        static uint32_t maxFrames = 10000;
        GuiUtils::FwInputUInt("Max Frames", &maxFrames);

        static uint32_t frameInc = 1;
        GuiUtils::FwInputUInt("Frame Increment", &frameInc);

        static char buffGraph[256];
        ImGui::InputText("Test Name", buffGraph, 256);

        static std::vector<std::string> testNamesForMulti;

        ImGui::SameLine();
        if (ImGui::Button("+"))
        {
            testNamesForMulti.emplace_back(buffGraph);
            buffGraph[0] = '\0';
        }

        if (isInActiveState)
            ImGui::BeginDisabled();
        {
            if (ImGui::Button("Compute Convergence"))
            {
                m_rmseTool.TriggerComputeConvergence(maxFrames, frameInc);
                ptFrameDirty = true;
            }

            const bool noConvergenceData = strcmp(buffGraph, "") == 0 || m_rmseTool.GetConvergenceSampleSize() == 0;

            if (noConvergenceData)
                ImGui::BeginDisabled();
            {
                if (ImGui::Button("Plot Convergence"))
                {
                    m_rmseTool.PlotConvergence(buffGraph);
                    m_rmseTool.ClearConvergenceData();
                }

                if (ImGui::Button("Save Test"))
                {
                    m_rmseTool.SaveTest(buffGraph);
                    m_rmseTool.ClearConvergenceData();
                }
            }
            if (noConvergenceData)
                ImGui::EndDisabled();

            ImGui::TextUnformatted("Multi-Convergence Test List:");
            ImGui::Indent(IM_GUI_INDENTATION);
            {
                for (int i = testNamesForMulti.size() - 1; i >= 0; i--)
                {
                    ImGui::Text("(%i) %s", i, testNamesForMulti[i].c_str());

                    ImGui::SameLine();
                    if (ImGui::Button(("-##" + testNamesForMulti[i]).c_str()))
                    {
                        testNamesForMulti.erase(testNamesForMulti.begin() + i);
                    }
                }
            }
            ImGui::Unindent(IM_GUI_INDENTATION);

            if (testNamesForMulti.size() < 2)
                ImGui::BeginDisabled();
            {
                static bool logPlot = false;
                ImGui::Checkbox("Log Plot", &logPlot);

                if (ImGui::Button("Plot Multi-Convergence"))
                {
                    m_rmseTool.PlotMultiConvergence(testNamesForMulti, logPlot);
                }
            }
            if (testNamesForMulti.size() < 2)
                ImGui::EndDisabled();
        }
        if (isInActiveState)
            ImGui::EndDisabled();

        if (!isInActiveState)
            ImGui::BeginDisabled();
        {
            if (ImGui::Button("Cancel"))
            {
                m_rmseTool.CancelOperation();
            }
        }
        if (!isInActiveState)
            ImGui::EndDisabled();

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Unindent(IM_GUI_INDENTATION);
    }

    if (config.FeatFlagEnabledDebug(eDebug_PathDumper))
    {
        if (ImGui::CollapsingHeader("Path Dump"))
        {
            ImGui::Indent(IM_GUI_INDENTATION);

            ImGui::Text("Pixel Coord:     (%i, %i)", m_dumpedPathPixelCoords.x, m_dumpedPathPixelCoords.y);
            ImGui::Text("Frame Index:     %i", m_dumpedPathFrameIdx);
            ImGui::Text("Camera Position: (%f, %f, %f)", m_dumpedPathCameraPosition.x, m_dumpedPathCameraPosition.y, m_dumpedPathCameraPosition.z);

            bool scheduleRun = false;

            if (ImGui::Button("Re-Run Existing Path (P)") || Input::IsKey(KeyCode::P))
            {
                m_scheduledRunFrameIdx = m_dumpedPathFrameIdx;
                scheduleRun = true;
            }

            if (ImGui::Button("Run New Path (O)") || Input::IsKey(KeyCode::O))
            {
                m_scheduledRunFrameIdx = m_dumpedPathFrameIdx + 1;
                scheduleRun = true;
            }

            if (scheduleRun)
            {
                m_isPathDumpAutomatic = false;

                m_scheduledRunPixelCoords = m_dumpedPathPixelCoords;
                m_scheduledRunCameraPosition = m_dumpedPathCameraPosition;
                m_scheduledRunViewMatrix = m_dumpedPathViewMatrix;

                if (config.FeatFlagEnabledCore(eCore_Accumulation))
                {
                    m_scheduledRunState = ScheduledRunState::eRecompilePipelineAndRunFrame;
                    SetFeatFlagCore(config.FlagsCore, eCore_Accumulation, false);
#if CHERRY_DEBUG_FEATURES_ENABLED
                    SetFeatFlagCore(config.DebugInfo.FeatFlagsCoreQS, eCore_Accumulation, false);
#endif
                }
                else
                    m_scheduledRunState = ScheduledRunState::eRunFrame;
            }

            ImGui::Checkbox("Automatic Dump", &m_isPathDumpAutomatic);

            uint32_t rowIncrementer = 0;
            for (int i = 0; i < static_cast<uint32_t>(PATH_DUMP_MAX_RAY_DEPTH); i++)
            {
                if (!m_cpuPathDump[i].Explored)
                    break;

                const std::string label = std::string("Ray ") + std::to_string(i);

                if (ImGui::TreeNode(label.c_str()))
                {
                    ImGui::Indent(IM_GUI_INDENTATION/2);

                    char buf[64];

                    constexpr float MIN_ROW_HEIGHT = 20.0f;
                    constexpr float COLOR_SQUARE_SIZE = 15.0f;

                    if (ImGui::BeginTable((label + "##table").c_str(), 3,
                                          ImGuiTableFlags_Borders |
                                          ImGuiTableFlags_RowBg |
                                          ImGuiTableFlags_SizingFixedFit))
                    {
                        ImGui::TableSetupColumn("Key", ImGuiTableColumnFlags_WidthFixed, 110.0f);
                        ImGui::TableSetupColumn("Value", ImGuiTableColumnFlags_WidthFixed, 170.0f);
                        ImGui::TableSetupColumn("", ImGuiTableColumnFlags_WidthFixed, 18.0f);
                        ImGui::TableHeadersRow();

                        auto RowBool = [&](const char* key, const uint32_t value)
                        {
                            ImGui::TableNextRow(0, MIN_ROW_HEIGHT);
                            ImGui::TableSetColumnIndex(0);
                            ImGui::TextUnformatted(key);

                            ImGui::TableSetColumnIndex(1);
                            const char* bVal = value == 0 ? "False" : "True";
                            snprintf(buf, sizeof(buf), "%s##bool-%i", bVal, rowIncrementer);
                            ImGui::Selectable(buf);
                            if (ImGui::IsItemClicked())
                                ImGui::SetClipboardText(std::string(buf).substr(0, std::string(buf).find('#')).c_str());

                            rowIncrementer++;
                        };

                        auto RowInt = [&](const char* key, const int value)
                        {
                            ImGui::TableNextRow(0, MIN_ROW_HEIGHT);
                            ImGui::TableSetColumnIndex(0);
                            ImGui::TextUnformatted(key);

                            ImGui::TableSetColumnIndex(1);
                            snprintf(buf, sizeof(buf), "%d##int-%i", value, rowIncrementer);
                            ImGui::Selectable(buf);
                            if (ImGui::IsItemClicked())
                                ImGui::SetClipboardText(std::string(buf).substr(0, std::string(buf).find('#')).c_str());

                            rowIncrementer++;
                        };

                        auto RowFloat = [&](const char* key, const float value, const bool isAssignedValue)
                        {
                            ImGui::TableNextRow(0, MIN_ROW_HEIGHT);
                            ImGui::TableSetColumnIndex(0);
                            if (!isAssignedValue)
                                ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(150, 150, 150, 150));
                            ImGui::TextUnformatted(key);
                            if (!isAssignedValue)
                                ImGui::PopStyleColor();

                            ImGui::TableSetColumnIndex(1);
                            if (isAssignedValue)
                                snprintf(buf, sizeof(buf), "%.6f##float-%i", value, rowIncrementer);
                            else
                                snprintf(buf, sizeof(buf), "-##float-%i", rowIncrementer);
                            ImGui::Selectable(buf);
                            if (ImGui::IsItemClicked())
                                ImGui::SetClipboardText(std::string(buf).substr(0, std::string(buf).find('#')).c_str());

                            ImGui::TableSetColumnIndex(2);
                            if (isAssignedValue)
                            {
                                const ImVec4 color(std::clamp(value, 0.0f, 1.0f), std::clamp(value, 0.0f, 1.0f), std::clamp(value, 0.0f, 1.0f), 1.0f);
                                ImGui::ColorButton((std::string("##color1-") + std::to_string(rowIncrementer)).c_str(), color, ImGuiColorEditFlags_NoTooltip | ImGuiColorEditFlags_NoDragDrop, ImVec2(COLOR_SQUARE_SIZE, COLOR_SQUARE_SIZE));
                            }
                            rowIncrementer++;
                        };

                        auto RowFloat3 = [&](const char* key, const hlsl::float3& v, const bool isAssignedValue)
                        {
                            ImGui::TableNextRow(0, MIN_ROW_HEIGHT);
                            ImGui::TableSetColumnIndex(0);
                            if (!isAssignedValue)
                                ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(150, 150, 150, 150));
                            ImGui::TextUnformatted(key);
                            if (!isAssignedValue)
                                ImGui::PopStyleColor();

                            ImGui::TableSetColumnIndex(1);
                            if (isAssignedValue)
                                snprintf(buf, sizeof(buf), "(%.3f, %.3f, %.3f)##float3-%i", v.x, v.y, v.z, rowIncrementer);
                            else
                                snprintf(buf, sizeof(buf), "-##float3-%i", rowIncrementer);
                            ImGui::Selectable(buf);
                            if (ImGui::IsItemClicked())
                                ImGui::SetClipboardText(std::string(buf).substr(0, std::string(buf).find('#')).c_str());

                            ImGui::TableSetColumnIndex(2);
                            if (isAssignedValue)
                            {
                                const ImVec4 color(std::clamp(v.x, 0.0f, 1.0f), std::clamp(v.y, 0.0f, 1.0f), std::clamp(v.z, 0.0f, 1.0f), 1.0f);
                                ImGui::ColorButton((std::string("##color3-") + std::to_string(rowIncrementer)).c_str(), color, ImGuiColorEditFlags_NoTooltip | ImGuiColorEditFlags_NoDragDrop, ImVec2(COLOR_SQUARE_SIZE, COLOR_SQUARE_SIZE));
                            }
                            rowIncrementer++;
                        };

                        auto RowSection = [&](const char* lab) -> bool
                        {
                            ImGui::TableNextRow();
                            ImGui::TableSetColumnIndex(0);

                            const std::string fullLabel = lab + std::string("##section-") + std::to_string(rowIncrementer);
                            rowIncrementer++;

                            return ImGui::TreeNodeEx(fullLabel.c_str(), ImGuiTreeNodeFlags_SpanAllColumns);
                        };

                        RowFloat3("ForcedOutput", m_cpuPathDump[i].ForcedOutput, true);

                        if (RowSection("Path State"))
                        {
                            RowInt("RaySegmentIdx", static_cast<int>(m_cpuPathDump[i].PathState.RaySegmentIdx));
                            RowFloat3("RayOrigin", m_cpuPathDump[i].PathState.Desc.Origin, true);
                            RowFloat3("RayDirection", m_cpuPathDump[i].PathState.Desc.Direction, true);
                            RowFloat3("Beta", m_cpuPathDump[i].PathState.Beta, true);
                            RowFloat3("Lo", m_cpuPathDump[i].PathState.Lo, true);
                            RowBool("LastRayWasDiracDelta", m_cpuPathDump[i].PathState.LastRayWasDiracDelta);
                            RowFloat("LastBxdfPdf", m_cpuPathDump[i].PathState.LastBxdfPdf, true);
                            ImGui::TreePop();
                        }

                        if (RowSection("Path Vertex Info"))
                        {
                            RowInt("Type", static_cast<int>(m_cpuPathDump[i].PathVertex.Type));
                            RowFloat3("Position", m_cpuPathDump[i].PathVertex.Position, true);
                            RowFloat3("Normal", m_cpuPathDump[i].PathVertex.SFrame.N, true);
                            RowFloat3("Wo", m_cpuPathDump[i].PathVertex.Wo, true);
                            RowFloat3("Wi", m_cpuPathDump[i].PathVertex.Wi, true);
                            RowFloat("Eta", m_cpuPathDump[i].PathVertex.Eta, true);
                            RowFloat3("Indirect", m_cpuPathDump[i].PathVertex.IndirectContribution, true);
                            RowFloat("PDF", m_cpuPathDump[i].PathVertex.PDF, true);
                            ImGui::TreePop();
                        }

                        std::string currentHeader;
                        bool currentHeaderOpen = false;

                        // Note: Starting from 1 due to ignored eDebugOutput_Disabled
                        for (int j = 1; j < static_cast<int>(DebugOutputIndex::eCount); ++j)
                        {
                            const hlsl::float3 value = m_cpuPathDumpOutputColor[i].Array[j].Value;
                            const bool isAssignedValue = m_cpuPathDumpOutputColor[i].Array[j].IsAssigned;

                            std::string name = s_debugOutputIdxNames[j];
                            const auto underscoreIdx = name.find_first_of('_');

                            if (underscoreIdx != std::string::npos)
                            {
                                const std::string newHeader = name.substr(0, underscoreIdx);
                                name = name.substr(underscoreIdx + 1);

                                if (currentHeader != newHeader)
                                {
                                    if (!currentHeader.empty() && currentHeaderOpen)
                                        ImGui::TreePop();

                                    currentHeader = newHeader;
                                    currentHeaderOpen = RowSection(currentHeader.c_str());
                                }
                            }
                            else if (!currentHeader.empty())
                            {
                                if (currentHeaderOpen)
                                    ImGui::TreePop();
                                currentHeader = "";
                                currentHeaderOpen = false;
                            }

                            if (!currentHeader.empty() && !currentHeaderOpen)
                                continue;

                            if (value.x == value.y && value.x == value.z)
                                RowFloat(name.c_str(), value.x, isAssignedValue);
                            else
                                RowFloat3(name.c_str(), value, isAssignedValue);
                        }

                        if (!currentHeader.empty() && currentHeaderOpen)
                            ImGui::TreePop();

                        ImGui::EndTable();
                    }

                    ImGui::Unindent(IM_GUI_INDENTATION/2);
                    ImGui::TreePop();
                }
            }

            ImGui::Unindent(IM_GUI_INDENTATION);
        }
    }

    std::vector<uint32_t> errors;
    if (config.FeatFlagEnabledDebug(eDebug_Asserts))
    {
        for (int i = 0; i < _countof(m_cpuErrorInfo); i++)
        {
            if (m_cpuErrorInfo[i].ExprCounter > 0 || m_cpuErrorInfo[i].NaNCounter > 0 || m_cpuErrorInfo[i].InfCounter > 0)
            {
                errors.emplace_back(i);
            }
        }
    }

    const std::string assertLabel = std::string("Assertion Errors (") + std::to_string(errors.size()) + ")";
    if (!errors.empty() && ImGui::CollapsingHeader(assertLabel.c_str()))
    {
        ImGui::Indent(IM_GUI_INDENTATION);
        for (int i = 0; i < errors.size(); i++)
        {
            if (i > 0)
                ImGui::Spacing();

            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(224 / 255., 82 / 255., 110 / 255., 255));

            const uint32_t dbgId = errors[i];
            ImGui::Text("%s: ", s_debugIdList[dbgId]);
            ImGui::SetItemTooltip("%s: ", s_debugIdList[dbgId]);
            ImGui::Indent(IM_GUI_INDENTATION);
            {
                ImGui::Text("PixelCoord: (%i, %i)", m_cpuErrorInfo[dbgId].PixelCoord.x, m_cpuErrorInfo[dbgId].PixelCoord.y);
                ImGui::Text("FrameIndex: %i", m_cpuErrorInfo[dbgId].FrameIndex);

                if (m_cpuErrorInfo[dbgId].ExprCounter > 0)
                {
                    ImGui::Text("EXPR=%u", m_cpuErrorInfo[dbgId].ExprCounter);
                    ImGui::SetItemTooltip("EXPR=%u", m_cpuErrorInfo[dbgId].ExprCounter);

                    ImGui::Indent(IM_GUI_INDENTATION);
                    auto printValue = [&](const char* name, const XMFLOAT4& value)
                    {
                        if (value.x == value.y && value.x == value.z && value.x == value.w)
                            ImGui::Text("%s=(%f)", name, value.x);
                        else
                            ImGui::Text("%s=(%f, %f, %f, %f)", name, value.x, value.y, value.z, value.w);
                    };

                    printValue("V1", m_cpuErrorInfo[dbgId].Value1);
                    printValue("V2", m_cpuErrorInfo[dbgId].Value2);
                    printValue("V3", m_cpuErrorInfo[dbgId].Value3);
                    ImGui::Unindent(IM_GUI_INDENTATION);
                }

                if (m_cpuErrorInfo[dbgId].NaNCounter > 0)
                {
                    ImGui::Text("NAN=%u", m_cpuErrorInfo[dbgId].NaNCounter);
                    ImGui::SetItemTooltip(" NAN=%u", m_cpuErrorInfo[dbgId].NaNCounter);
                }

                if (m_cpuErrorInfo[dbgId].InfCounter > 0)
                {
                    ImGui::Text("INF=%u", m_cpuErrorInfo[dbgId].InfCounter);
                    ImGui::SetItemTooltip(" INF=%u", m_cpuErrorInfo[dbgId].InfCounter);
                }

                ImGui::PopStyleColor();

                if (ImGui::Button((std::string("Dump Path##") + std::to_string(dbgId)).c_str()))
                {
                    m_isPathDumpAutomatic = false;

                    m_scheduledRunPixelCoords = m_cpuErrorInfo[dbgId].PixelCoord;
                    m_scheduledRunFrameIdx = m_cpuErrorInfo[dbgId].FrameIndex;
                    m_scheduledRunCameraPosition = m_cpuErrorInfo[dbgId].CameraPositionWorld;

                    m_scheduledRunViewMatrix = XMMatrixInverse(nullptr, XMLoadFloat4x4(&m_cpuErrorInfo[dbgId].InvV));

                    if (config.FeatFlagEnabledCore(eCore_Accumulation) || !config.FeatFlagEnabledDebug(eDebug_PathDumper))
                    {
                        m_scheduledRunState = ScheduledRunState::eRecompilePipelineAndRunFrame;
                        SetFeatFlagCore(config.FlagsCore, eCore_Accumulation, false);
#if CHERRY_DEBUG_FEATURES_ENABLED
                        SetFeatFlagCore(config.DebugInfo.FeatFlagsCoreQS, eCore_Accumulation, false);
                        SetFeatFlagDebug(config.DebugInfo.FlagsDebug, eDebug_PathDumper, true);
                        SetFeatFlagDebug(config.DebugInfo.FeatFlagsDebugQS, eDebug_PathDumper, true);
#endif
                    }
                    else
                        m_scheduledRunState = ScheduledRunState::eRunFrame;
                }
            }
            ImGui::Unindent(IM_GUI_INDENTATION);
        }

        if (ImGui::Button("Clear Assertions"))
            m_scheduleClearErrors = true;

        ImGui::Unindent(IM_GUI_INDENTATION);
        ImGui::Spacing();
    }

#if CHERRY_DEBUG_FEATURES_ENABLED
    if (config.FeatFlagEnabledDebug(eDebug_OutputColor) && ImGui::CollapsingHeader("Debug Output Colors"))
    {
        ImGui::Indent(IM_GUI_INDENTATION);

        ptFrameDirty |= ImGui::InputInt("Chosen Ray Depth", &config.DebugInfo.ChosenRayDepth);
        ImGui::IsItemDeactivatedAfterEdit();
        config.DebugInfo.ChosenRayDepth = max(-1, config.DebugInfo.ChosenRayDepth);

        ImGui::Text("Remap:");
        ImGui::Indent(IM_GUI_INDENTATION);
        if (ImGui::BeginTable("Debug Output Color Remaps", 3))
        {
            const auto remapAsUint = static_cast<uint32_t>(config.DebugInfo.OutputColorRemap);
            for (int i = 0; i < _countof(s_debugOutputColorRemapNames); i++)
            {
                ImGui::TableNextColumn();

                const uint32_t flag = 1 << i;
                bool isEnabled = remapAsUint & flag;
                const bool prevIsEnabled = isEnabled;
                const bool clicked = ImGui::Checkbox(s_debugOutputColorRemapNames[i], &isEnabled);
                ptFrameDirty |= clicked;
                ImGui::IsItemDeactivatedAfterEdit();

                if (clicked && isEnabled)
                    config.DebugInfo.OutputColorRemap = static_cast<DebugOutputColorRemap>(remapAsUint | flag);
                else if (clicked && prevIsEnabled)
                    config.DebugInfo.OutputColorRemap = static_cast<DebugOutputColorRemap>(remapAsUint ^ flag);

                ImGui::SetItemTooltip("%s", s_debugOutputColorRemapNames[i]);
            }
        }
        ImGui::Unindent(IM_GUI_INDENTATION);
        ImGui::EndTable();

        if (ImGui::BeginTable("Debug Outputs", 2))
        {
            static int e = static_cast<int>(config.DebugInfo.OutputColorIdx);
            int c = 0;
            for (auto& debugOutputIdxName : s_debugOutputIdxNames)
            {
                ImGui::TableNextColumn();
                ptFrameDirty |= ImGui::RadioButton(debugOutputIdxName, &e, c++);
                ImGui::IsItemDeactivatedAfterEdit();
                ImGui::SetItemTooltip("%s", debugOutputIdxName);
            }
            config.DebugInfo.OutputColorIdx = static_cast<DebugOutputIndex>(e);
        }

        ImGui::Unindent(IM_GUI_INDENTATION);
        ImGui::EndTable();
        ImGui::Spacing();
    }
#endif
}