#include "System/pch.h"
#include "Deferred.h"

#include "GreenhouseConfig.h"
#include "System/GuiUtils.h"
#include "Utils/ConstantsCpp.h"

void Deferred::GUI(GreenhouseConfig* config, bool& frameDirty)
{
    bool shadowMatricesDirty = false;

    GuiUtils::FwInputInt("Bloom Radius", &config->RasterConfig.BloomRadius);
    GuiUtils::FwInputFloat("Bloom Threshold", &config->RasterConfig.BloomLumThreshold);
    GuiUtils::FwInputUInt("Bloom Downsample Count", &config->RasterConfig.BloomDownsampleCount);

    ImGui::SeparatorText("Flags");
    ImGui::Indent(IM_GUI_INDENTATION);
    {
        ImGui::Checkbox("Environment Map", &config->RasterConfig.FlagEnabledEnvironmentMap);
        ImGui::Checkbox("PunctualLights", &config->RasterConfig.FlagEnabledPunctualLights);
        ImGui::Checkbox("DirectionalLight", &config->RasterConfig.FlagEnabledDirectionalLight);
        shadowMatricesDirty |= ImGui::Checkbox("ShadowMaps", &config->RasterConfig.FlagEnabledShadowMaps);
        ImGui::Checkbox("Ambient", &config->RasterConfig.FlagEnabledAmbient);
    }
    ImGui::Unindent(IM_GUI_INDENTATION);

    if (config->RasterConfig.FlagEnabledShadowMaps)
    {
        ImGui::SeparatorText("Shadow Map");
        ImGui::Indent(IM_GUI_INDENTATION);
        {
            shadowMatricesDirty |= GuiUtils::FwInputUInt("Cascade Count", &config->RasterConfig.SmCascadeCount);

            shadowMatricesDirty |= GuiUtils::FwInputFloat("Width##SM", &config->RasterConfig.SmWidth);
            shadowMatricesDirty |= GuiUtils::FwInputFloat("Height##SM", &config->RasterConfig.SmHeight);
            shadowMatricesDirty |= GuiUtils::FwInputFloat("Near Plane##SM", &config->RasterConfig.SmNear);
            shadowMatricesDirty |= GuiUtils::FwInputFloat("Far Plane##SM", &config->RasterConfig.SmFar);
            shadowMatricesDirty |= GuiUtils::FwInputFloat("Distance##SM", &config->RasterConfig.SmDirDist);

            GuiUtils::FwInputFloat("Depth Bias Distant##SM", &config->RasterConfig.SmBiasDepthDistant, 0, 0, "%f");
            GuiUtils::FwInputFloat("Normal Bias Distant##SM", &config->RasterConfig.SmBiasNormalDistant, 0, 0, "%f");

            GuiUtils::FwInputFloat("Depth Bias Point##SM", &config->RasterConfig.SmBiasDepthPoint, 0, 0, "%f");
            GuiUtils::FwInputFloat("Normal Bias Point##SM", &config->RasterConfig.SmBiasNormalPoint, 0, 0, "%f");

            GuiUtils::FwInputUInt("PCF Count", &config->RasterConfig.SmPcfCount);
            GuiUtils::FwInputFloat("PCF Radius##SM", &config->RasterConfig.SmPcfRadius);
        }
        ImGui::Unindent(IM_GUI_INDENTATION);
    }

    if (shadowMatricesDirty)
        m_shadowMapManager.MarkMatricesDirty();
}
