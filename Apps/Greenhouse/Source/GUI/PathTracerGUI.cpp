#include "System/pch.h"
#include "PathTracer.h"
#include "Utils/ConstantsCpp.h"

#include "imgui.h"
#include "GreenhouseConfig.h"

#if CHERRY_DEBUG_FEATURES_ENABLED
void PathTracer::RenderGUI_DebugInfo(PathTracerConfig& config)
{
    if (GetPathTracerDebugFlag(config.DebugInfo.Flags, eDebug_PathDumper))
    {
        if (ImGui::CollapsingHeader("Path Dump"))
        {
            ImGui::Indent(IM_GUI_INDENTATION);

            ImGui::Text("Pixel Coord:     (%i, %i)", m_dumpedPathPixelCoords.x, m_dumpedPathPixelCoords.y);
            ImGui::Text("Frame Index:     %i", m_dumpedPathFrameIdx);
            ImGui::Text("Camera Position: (%f, %f, %f)", m_dumpedPathCameraPosition.x, m_dumpedPathCameraPosition.y, m_dumpedPathCameraPosition.z);

            if (ImGui::Button("Re-Run Path"))
            {
                m_scheduledRunState = ScheduledRunState::eRunFrame;
                m_isPathDumpAutomatic = false;

                m_scheduledRunPixelCoords = m_dumpedPathPixelCoords;
                m_scheduledRunFrameIdx = m_dumpedPathFrameIdx;
                m_scheduledRunCameraPosition = m_dumpedPathCameraPosition;
                m_scheduledRunViewMatrix = m_dumpedPathViewMatrix;
            }

            if (!m_isPathDumpAutomatic && ImGui::Button("Enable Automatic Path Dump"))
                m_isPathDumpAutomatic = true;

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

                        RowInt("Ray Segment", static_cast<int>(m_cpuPathDump[i].PathState.RaySegmentIdx));
                        RowFloat3("Ray Origin", m_cpuPathDump[i].PathState.Desc.Origin, true);
                        RowFloat3("Ray Direction", m_cpuPathDump[i].PathState.Desc.Direction, true);
                        RowFloat3("Beta", m_cpuPathDump[i].PathState.Beta, true);
                        RowFloat3("Lo", m_cpuPathDump[i].PathState.Lo, true);
                        RowFloat3("Gradient", m_cpuPathDump[i].PathState.Gradient, true);
                        RowInt("Last Ray Was Dirac Delta", m_cpuPathDump[i].PathState.LastRayWasDiracDelta);
                        RowFloat("Last Ray PDF", m_cpuPathDump[i].PathState.LastBxdfPdf, true);

                        for (int j = 1; j < static_cast<int>(DebugOutputIndex::eCount); ++j) // Starting from 1 due to ignored eDebugOutput_Disabled
                        {
                            const bool isAssignedValue = m_cpuPathDump[i].DebugOutputs.IsAssignedValueList[j];

                            if (m_cpuPathDump[i].DebugOutputs.Float3List[j].x == m_cpuPathDump[i].DebugOutputs.Float3List[j].y && m_cpuPathDump[i].DebugOutputs.Float3List[j].x == m_cpuPathDump[i].DebugOutputs.Float3List[j].z)
                                RowFloat(s_debugOutputIdxNames[j], m_cpuPathDump[i].DebugOutputs.Float3List[j].x, isAssignedValue);
                            else
                                RowFloat3(s_debugOutputIdxNames[j], m_cpuPathDump[i].DebugOutputs.Float3List[j], isAssignedValue);
                        }

                        ImGui::EndTable();
                    }

                    ImGui::Unindent(IM_GUI_INDENTATION/2);
                    ImGui::TreePop();
                }
            }

            ImGui::Unindent(IM_GUI_INDENTATION);
        }
    }

    if (!GetPathTracerDebugFlag(config.DebugInfo.Flags, eDebug_Asserts))
        return;

    std::vector<uint32_t> errors;
    for (int i = 0; i < _countof(m_cpuErrorInfo); i++)
    {
        if (m_cpuErrorInfo[i].ExprCounter > 0 || m_cpuErrorInfo[i].NaNCounter > 0 || m_cpuErrorInfo[i].InfCounter > 0)
        {
            errors.emplace_back(i);
        }
    }

    if (errors.empty())
        return;

    const std::string assertLabel = std::string("Assertion Errors (") + std::to_string(errors.size()) + ")";
    if (ImGui::CollapsingHeader(assertLabel.c_str()))
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
                    m_scheduledRunState = ScheduledRunState::eRunFrame;
                    m_isPathDumpAutomatic = false;

                    m_scheduledRunPixelCoords = m_cpuErrorInfo[dbgId].PixelCoord;
                    m_scheduledRunFrameIdx = m_cpuErrorInfo[dbgId].FrameIndex;
                    m_scheduledRunCameraPosition = m_cpuErrorInfo[dbgId].CameraPositionWorld;

                    m_scheduledRunViewMatrix = XMMatrixInverse(nullptr, XMLoadFloat4x4(&m_cpuErrorInfo[dbgId].InvV));

                    SetPathTracerDebugFlag(config.DebugInfo.Flags, eDebug_PathDumper, true);
                }
            }
            ImGui::Unindent(IM_GUI_INDENTATION);
        }

        if (ImGui::Button("Clear Assertions"))
            m_scheduleClearErrors = true;

        ImGui::Unindent(IM_GUI_INDENTATION);
    }
    ImGui::Spacing();
}
#endif