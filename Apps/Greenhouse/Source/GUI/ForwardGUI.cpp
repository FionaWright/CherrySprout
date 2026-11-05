#include "System/pch.h"
#include "Forward.h"

#include "GreenhouseConfig.h"
#include "System/GuiUtils.h"
#include "Utils/ConstantsCpp.h"

void Forward::GUI(GreenhouseConfig* config, bool& frameDirty)
{
    bool shadowMatricesDirty = false;

    ImGui::SeparatorText("Flags");
    ImGui::Indent(IM_GUI_INDENTATION);
    {
        ImGui::Checkbox("Environment Map", &config->ForwardConfig.FlagEnabledEnvironmentMap);
        ImGui::Checkbox("PunctualLights", &config->ForwardConfig.FlagEnabledPunctualLights);
        ImGui::Checkbox("DirectionalLight", &config->ForwardConfig.FlagEnabledDirectionalLight);
        shadowMatricesDirty |= ImGui::Checkbox("ShadowMaps", &config->ForwardConfig.FlagEnabledShadowMaps);
        ImGui::Checkbox("Ambient", &config->ForwardConfig.FlagEnabledAmbient);
    }
    ImGui::Unindent(IM_GUI_INDENTATION);

    if (config->ForwardConfig.FlagEnabledShadowMaps)
    {
        ImGui::SeparatorText("Shadow Map");
        ImGui::Indent(IM_GUI_INDENTATION);
        {
            shadowMatricesDirty |= GuiUtils::FwInputUInt("Cascade Count", &config->ForwardConfig.SmCascadeCount);

            shadowMatricesDirty |= GuiUtils::FwInputFloat("Width##SM", &config->ForwardConfig.SmWidth);
            shadowMatricesDirty |= GuiUtils::FwInputFloat("Height##SM", &config->ForwardConfig.SmHeight);
            shadowMatricesDirty |= GuiUtils::FwInputFloat("Near Plane##SM", &config->ForwardConfig.SmNear);
            shadowMatricesDirty |= GuiUtils::FwInputFloat("Far Plane##SM", &config->ForwardConfig.SmFar);
            shadowMatricesDirty |= GuiUtils::FwInputFloat("Distance##SM", &config->ForwardConfig.SmDirDist);

            GuiUtils::FwInputFloat("Depth Bias##SM", &config->ForwardConfig.SmBiasDepth, 0, 0, "%f");
            GuiUtils::FwInputFloat("Normal Bias##SM", &config->ForwardConfig.SmBiasNormal, 0, 0, "%f");

            GuiUtils::FwInputUInt("PCF Count", &config->ForwardConfig.SmPcfCount);
            GuiUtils::FwInputFloat("PCF Radius##SM", &config->ForwardConfig.SmPcfRadius);
        }
        ImGui::Unindent(IM_GUI_INDENTATION);
    }

    if (shadowMatricesDirty)
        m_shadowMapManager.MarkMatricesDirty();
}
