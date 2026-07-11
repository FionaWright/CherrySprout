//
// Created by fionaw on 26/10/2025.
//

#ifndef PT_HOTRELOADER_H
#define PT_HOTRELOADER_H

#include <filesystem>
#include <string>
#include <vector>

#include "HWI/Pipeline.h"

class D3D;

struct ShaderEntry
{
    std::string ID;
    std::string Filepath;
    std::filesystem::file_time_type Timestamp;
};

struct GraphicsPipelineEntry
{
    ShaderEntry VertexEntry, PixelEntry;

    Pipeline* Ptr;
    D3D12_GRAPHICS_PIPELINE_STATE_DESC Desc;
    std::vector<D3D12_INPUT_ELEMENT_DESC> InputLayout;
    std::vector<std::string> CompileArgs;
};

struct ComputePipelineEntry
{
    ShaderEntry ComputeEntry;

    Pipeline* Ptr;
    D3D12_COMPUTE_PIPELINE_STATE_DESC Desc;
    std::vector<std::string> CompileArgs;
};

enum class ReloadMode : uint32_t
{
    eAll,
    eGraphics,
    eCompute,
};

class HotReloader
{
public:
    static void TrackGraphicsPipeline(const char* vsID, const char* psID, Pipeline* ptr, const D3D12_GRAPHICS_PIPELINE_STATE_DESC& desc, const std::vector<std::string>& compileArgs);
    static void TrackComputePipeline(const char* csID, Pipeline* ptr, const D3D12_COMPUTE_PIPELINE_STATE_DESC& desc, const std::vector<std::string>& compileArgs);
    static void ReloadPipelines(D3D* d3d, bool onlyModified = true, ReloadMode reloadMode = ReloadMode::eAll);

private:
    static std::vector<GraphicsPipelineEntry> s_graphicsPipelines;
    static std::vector<ComputePipelineEntry> s_computePipelines;
};


#endif //PT_HOTRELOADER_H
