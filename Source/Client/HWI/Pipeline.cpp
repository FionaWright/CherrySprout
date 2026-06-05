//
// Created by fiona on 25/09/2025.
//

#include "System/pch.h"
#include "HWI/Pipeline.h"

#include "System/Config.h"
#include "System/FileHelper.h"
#include "Utils/Helper.h"

#include "HWI/CompileShaderDXC.h"
#include "Utils/D3DUtils.h"

#ifdef _DEBUG
#   include "Debug/HotReloader.h"
#endif

void Pipeline::InitGraphics(ID3D12Device* device, const char* vs, const char* ps,
                            D3D12_GRAPHICS_PIPELINE_STATE_DESC& desc, const std::vector<std::string>& compileArgs)
{
    SetGraphics(device, vs, ps, desc, compileArgs);

#ifdef _DEBUG
    HotReloader::TrackGraphicsPipeline(vs, ps, this, desc, compileArgs);
#endif
}

void Pipeline::SetGraphics(ID3D12Device* device, const char* vs, const char* ps,
                            D3D12_GRAPHICS_PIPELINE_STATE_DESC& desc, const std::vector<std::string>& compileArgs)
{
#if defined(_DEBUG)
    constexpr ShaderCompileFlags compileFlags = static_cast<ShaderCompileFlags>(SCF_Debug | SCF_DisableOptimize);
#else
    constexpr ShaderCompileFlags compileFlags = 0;
#endif

    m_pso = nullptr;

    const std::string vsPath = FileHelper::GetAssetShaderFullPath(vs);
    const std::string psPath = FileHelper::GetAssetShaderFullPath(ps);

    const ComPtr<IDxcBlob> blobV = CompileShaderDXC(vsPath, "VSMain", "vs_6_6", compileFlags, compileArgs);
    const ComPtr<IDxcBlob> blobP = CompileShaderDXC(psPath, "PSMain", "ps_6_6", compileFlags, compileArgs);

    desc.VS = {blobV->GetBufferPointer(), blobV->GetBufferSize()};
    desc.PS = {blobP->GetBufferPointer(), blobP->GetBufferSize()};

    V(device->CreateGraphicsPipelineState(&desc, IID_PPV_ARGS(&m_pso)));
}

void Pipeline::InitCompute(ID3D12Device* device, const char* cs, ID3D12RootSignature* rootSig,
                           const std::vector<std::string>& compileArgs)
{
    auto desc = CreateComputePipelineDesc(rootSig);
    InitCompute(device, cs, desc, compileArgs);
}

void Pipeline::InitCompute(ID3D12Device* device, const char* cs, D3D12_COMPUTE_PIPELINE_STATE_DESC& desc,
                           const std::vector<std::string>& compileArgs)
{
    SetCompute(device, cs, desc, compileArgs);

#ifdef _DEBUG
    HotReloader::TrackComputePipeline(cs, this, desc, compileArgs);
#endif
}

void Pipeline::SetCompute(ID3D12Device* device, const char* cs, D3D12_COMPUTE_PIPELINE_STATE_DESC& desc,
                           const std::vector<std::string>& compileArgs)
{
#if defined(_DEBUG)
    constexpr ShaderCompileFlags compileFlags = static_cast<ShaderCompileFlags>(SCF_Debug | SCF_DisableOptimize);
#else
    constexpr ShaderCompileFlags compileFlags = 0;
#endif

    const std::string csPath = FileHelper::GetAssetShaderFullPath(cs);
    ComPtr<IDxcBlob> blobC = CompileShaderDXC(csPath, "CSMain", "cs_6_6", compileFlags, compileArgs);

    desc.CS = {blobC->GetBufferPointer(), blobC->GetBufferSize()};
    V(device->CreateComputePipelineState(&desc, IID_PPV_ARGS(&m_pso)));
}