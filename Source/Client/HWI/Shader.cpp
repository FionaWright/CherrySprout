//
// Created by fiona on 25/09/2025.
//

#include "System/pch.h"
#include "HWI/Shader.h"

#include "System/Config.h"
#include "System/FileHelper.h"
#include "Utils/Helper.h"

#include "HWI/CompileShaderDXC.h"

#ifdef _DEBUG
//#   include "../../../Headers/client/Debug/HotReloader.h"
#endif

void Shader::InitVsPs(const char* vs, const char* ps, D3D12_INPUT_LAYOUT_DESC ild, ID3D12Device* device,
                      ID3D12RootSignature* rootSig, const bool dsvEnabled, const std::vector<std::string>& args,
                      const uint32_t numRTVs, const D3D12_PRIMITIVE_TOPOLOGY_TYPE topology)
{
#if defined(_DEBUG)
    constexpr ShaderCompileFlags compileFlags = static_cast<ShaderCompileFlags>(SCF_Debug | SCF_DisableOptimize);
#else
    constexpr ShaderCompileFlags compileFlags = 0;
#endif

    m_pso = nullptr;

    const std::string vsPath = FileHelper::GetAssetShaderFullPath(vs);
    const std::string psPath = FileHelper::GetAssetShaderFullPath(ps);

    ComPtr<IDxcBlob> blobV = CompileShaderDXC(vsPath, "VSMain", "vs_6_6", compileFlags, args);
    ComPtr<IDxcBlob> blobP = CompileShaderDXC(psPath, "PSMain", "ps_6_6", compileFlags, args);

    D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
    psoDesc.InputLayout = ild;
    psoDesc.pRootSignature = rootSig;
    psoDesc.VS = {blobV->GetBufferPointer(), blobV->GetBufferSize()};
    psoDesc.PS = {blobP->GetBufferPointer(), blobP->GetBufferSize()};
    psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
    psoDesc.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;
    psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
    psoDesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
    psoDesc.DSVFormat = DXGI_FORMAT_D32_FLOAT;
    psoDesc.DepthStencilState.DepthEnable = dsvEnabled ? TRUE : FALSE;
    psoDesc.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;
    psoDesc.DepthStencilState.StencilEnable = dsvEnabled ? TRUE : FALSE;
    psoDesc.SampleMask = UINT_MAX;
    psoDesc.PrimitiveTopologyType = topology;
    psoDesc.NumRenderTargets = numRTVs;
    for (int i = 0; i < numRTVs; i++)
        psoDesc.RTVFormats[i] = Config::GetRender().RtvFormat;
    psoDesc.SampleDesc.Count = 1;
    V(device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&m_pso)));

#ifdef _DEBUG
    // if (m_assignedToHotReload)
    // {
    //     HotReloader::UpdateShaderVsPs(vs, ps, this, ild, rootSig, dsvEnabled, args, numRTVs, topology);
    //     return;
    // }
    // HotReloader::AssignShaderVsPs(vs, ps, this, ild, rootSig, dsvEnabled, args, numRTVs, topology);
    // m_assignedToHotReload = true;
#endif
}

void Shader::InitCs(const char* cs, ID3D12Device* device, ID3D12RootSignature* rootSig,
                    const std::vector<std::string>& args)
{
#if defined(_DEBUG)
    constexpr ShaderCompileFlags compileFlags = static_cast<ShaderCompileFlags>(SCF_Debug | SCF_DisableOptimize);
#else
    constexpr ShaderCompileFlags compileFlags = 0;
#endif

    const std::string csPath = FileHelper::GetAssetShaderFullPath(cs);
    ComPtr<IDxcBlob> blobC = CompileShaderDXC(csPath, "CSMain", "cs_6_6", compileFlags, args);

    D3D12_COMPUTE_PIPELINE_STATE_DESC psoDesc = {};
    psoDesc.pRootSignature = rootSig;
    psoDesc.CS = {blobC->GetBufferPointer(), blobC->GetBufferSize()};
    V(device->CreateComputePipelineState(&psoDesc, IID_PPV_ARGS(&m_pso)));

#ifdef _DEBUG
    // if (m_assignedToHotReload)
    // {
    //     HotReloader::UpdateShaderCs(cs, this, rootSig);
    //     return;
    // }
    // HotReloader::AssignShaderCs(cs, this, rootSig);
    // m_assignedToHotReload = true;
#endif
}
