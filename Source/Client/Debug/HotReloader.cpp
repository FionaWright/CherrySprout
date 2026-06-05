//
// Created by fionaw on 26/10/2025.
//

#include "System/pch.h"

#include "Debug/HotReloader.h"

#include "HWI/D3D.h"
#include "Utils/Helper.h"
#include "System/FileHelper.h"

std::vector<GraphicsPipelineEntry> HotReloader::s_graphicsPipelines;
std::vector<ComputePipelineEntry> HotReloader::s_computePipelines;

void HotReloader::TrackGraphicsPipeline(const char* vsID, const char* psID, Pipeline* ptr,
                                        const D3D12_GRAPHICS_PIPELINE_STATE_DESC desc,
                                        const std::vector<std::string> compileArgs)
{
    const std::string vsPath = FileHelper::GetAssetShaderFullPath(vsID);
    const std::string psPath = FileHelper::GetAssetShaderFullPath(psID);

    if (!std::filesystem::exists(vsPath) || !std::filesystem::exists(psPath))
        throw std::exception("Path Invalid");

    const ShaderEntry vsEntry = {vsID, vsPath, std::filesystem::last_write_time(vsPath)};
    const ShaderEntry psEntry = {psID, psPath, std::filesystem::last_write_time(psPath)};
    GraphicsPipelineEntry entry = {vsEntry, psEntry, ptr, desc, compileArgs};
    s_graphicsPipelines.emplace_back(entry);

    CherryPrint("Hot Reloader Tracking Graphics Pipeline: " << entry.VertexEntry.ID << ", " << entry.PixelEntry.ID);
}

void HotReloader::TrackComputePipeline(const char* csID, Pipeline* ptr, const D3D12_COMPUTE_PIPELINE_STATE_DESC desc,
                                       const std::vector<std::string> compileArgs)
{
    const std::string csPath = FileHelper::GetAssetShaderFullPath(csID);

    if (!std::filesystem::exists(csPath))
        throw std::exception("Path Invalid");

    const ShaderEntry csEntry = {csID, csPath, std::filesystem::last_write_time(csPath)};
    ComputePipelineEntry entry = {csEntry, ptr, desc, compileArgs};
    s_computePipelines.emplace_back(entry);

    CherryPrint("Hot Reloader Tracking Compute Pipeline: " << entry.ComputeEntry.ID);
}

bool ShaderEntryUpdated(const ShaderEntry& entry)
{
    return entry.Timestamp != std::filesystem::last_write_time(entry.Filepath);
}

void HotReloader::ReloadPipelines(D3D* d3d, const bool onlyModified, const ReloadMode reloadMode)
{
    std::vector<uint32_t> dirtyGraphicsPipelines;
    std::vector<uint32_t> dirtyComputePipelines;

    if (reloadMode != ReloadMode::eCompute)
    {
        for (int i = 0; i < s_graphicsPipelines.size(); i++)
        {
            const auto& entry = s_graphicsPipelines[i];
            if (ShaderEntryUpdated(entry.VertexEntry) || ShaderEntryUpdated(entry.PixelEntry) || !onlyModified)
                dirtyGraphicsPipelines.emplace_back(i);
        }
    }

    if (reloadMode != ReloadMode::eGraphics)
    {
        for (int i = 0; i < s_computePipelines.size(); i++)
        {
            const auto& entry = s_computePipelines[i];
            if (ShaderEntryUpdated(entry.ComputeEntry) || !onlyModified)
                dirtyComputePipelines.emplace_back(i);
        }
    }

    if (dirtyGraphicsPipelines.size() == 0 && dirtyComputePipelines.size() == 0)
        return;

    d3d->Flush();

    for (int i = 0; i < dirtyGraphicsPipelines.size(); i++)
    {
        auto& entry = s_graphicsPipelines[i];

        CherryPrint("Hot Reloading Graphics Pipeline: " << entry.VertexEntry.ID << ", " << entry.PixelEntry.ID);

        entry.Ptr->SetGraphics(d3d->GetDevice(), entry.VertexEntry.ID.c_str(),
                                entry.PixelEntry.ID.c_str(), entry.Desc, entry.CompileArgs);

        entry.VertexEntry.Timestamp = std::filesystem::last_write_time(entry.VertexEntry.Filepath);
        entry.PixelEntry.Timestamp = std::filesystem::last_write_time(entry.PixelEntry.Filepath);
    }

    for (int i = 0; i < dirtyComputePipelines.size(); i++)
    {
        auto& entry = s_computePipelines[i];

        CherryPrint("Hot Reloading Compute Pipeline: " << entry.ComputeEntry.ID);

        entry.Ptr->SetCompute(d3d->GetDevice(), entry.ComputeEntry.ID.c_str(), entry.Desc, entry.CompileArgs);

        entry.ComputeEntry.Timestamp = std::filesystem::last_write_time(entry.ComputeEntry.Filepath);
    }
}
