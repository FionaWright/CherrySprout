#ifndef H_D3D_UTILS_H
#define H_D3D_UTILS_H

#include "System/Config.h"
#include "HWI/D12Resource.h"
#include "HWI/D3D.h"
#include "HWI/Heap.h"

inline size_t Align(const size_t value, const size_t alignment)
{
    return (value + alignment - 1) & ~(alignment - 1);
}

inline void InitializeSamplerPointClamp(D3D12_STATIC_SAMPLER_DESC* desc)
{
    *desc = {};

    desc->AddressU = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
    desc->AddressV = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
    desc->AddressW = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;

    desc->Filter = D3D12_FILTER_MIN_MAG_MIP_POINT;
    desc->ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
    desc->ShaderRegister = 0;
}

inline void InitializeSamplerLinearClamp(D3D12_STATIC_SAMPLER_DESC* desc)
{
    *desc = {};

    desc->AddressU = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
    desc->AddressV = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
    desc->AddressW = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;

    desc->Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
    desc->ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
    desc->ShaderRegister = 0;
}

inline void InitializeSamplerPointWrap(D3D12_STATIC_SAMPLER_DESC* desc)
{
    *desc = {};

    desc->AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
    desc->AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
    desc->AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;

    desc->Filter = D3D12_FILTER_MIN_MAG_MIP_POINT;
    desc->ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
    desc->ShaderRegister = 0;
}

inline void InitializeSamplerLinearWrap(D3D12_STATIC_SAMPLER_DESC* desc)
{
    *desc = {};

    desc->AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
    desc->AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
    desc->AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;

    desc->Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
    desc->ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
    desc->ShaderRegister = 0;
}

inline void InitializeSamplerAnisotropicWrap(
    D3D12_STATIC_SAMPLER_DESC* desc,
    UINT maxAnisotropy = 16)
{
    *desc = {};

    desc->AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
    desc->AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
    desc->AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;

    desc->Filter = D3D12_FILTER_ANISOTROPIC;
    desc->MaxAnisotropy = maxAnisotropy;

    desc->ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
    desc->ShaderRegister = 0;
}

inline void InitializeSamplerAnisotropicClamp(
    D3D12_STATIC_SAMPLER_DESC* desc,
    UINT maxAnisotropy = 16)
{
    *desc = {};

    desc->AddressU = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
    desc->AddressV = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
    desc->AddressW = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;

    desc->Filter = D3D12_FILTER_ANISOTROPIC;
    desc->MaxAnisotropy = maxAnisotropy;

    desc->ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
    desc->ShaderRegister = 0;
}

inline size_t FormatBitsPerPixel(_In_ const DXGI_FORMAT fmt, bool& isBC)
{
    switch (fmt)
    {
    case DXGI_FORMAT_R32G32B32A32_FLOAT:
    case DXGI_FORMAT_R32G32B32A32_UINT:
    case DXGI_FORMAT_R32G32B32A32_SINT:
        return 128;

    case DXGI_FORMAT_R32G32B32_FLOAT:
    case DXGI_FORMAT_R32G32B32_UINT:
    case DXGI_FORMAT_R32G32B32_SINT:
        return 96;

    case DXGI_FORMAT_R16G16B16A16_FLOAT:
    case DXGI_FORMAT_R16G16B16A16_UNORM:
    case DXGI_FORMAT_R16G16B16A16_UINT:
    case DXGI_FORMAT_R16G16B16A16_SNORM:
    case DXGI_FORMAT_R16G16B16A16_SINT:
    case DXGI_FORMAT_R32G32_FLOAT:
    case DXGI_FORMAT_R32G32_UINT:
    case DXGI_FORMAT_R32G32_SINT:
        return 64;

    case DXGI_FORMAT_R8G8B8A8_UNORM:
    case DXGI_FORMAT_R8G8B8A8_UINT:
    case DXGI_FORMAT_R8G8B8A8_SNORM:
    case DXGI_FORMAT_R8G8B8A8_SINT:
        return 32;

    case DXGI_FORMAT_R8_UNORM:
    case DXGI_FORMAT_R8_UINT:
    case DXGI_FORMAT_R8_SNORM:
    case DXGI_FORMAT_R8_SINT:
        return 8;

    case DXGI_FORMAT_BC1_UNORM:
    case DXGI_FORMAT_BC1_UNORM_SRGB:
        isBC = true; return 8;
    case DXGI_FORMAT_BC2_UNORM:
    case DXGI_FORMAT_BC2_UNORM_SRGB:
    case DXGI_FORMAT_BC3_UNORM:
    case DXGI_FORMAT_BC3_UNORM_SRGB:
        isBC = true; return 16;
    case DXGI_FORMAT_BC5_UNORM:
    case DXGI_FORMAT_BC6H_UF16:
    case DXGI_FORMAT_BC6H_SF16:
    case DXGI_FORMAT_BC7_UNORM:
    case DXGI_FORMAT_BC7_UNORM_SRGB:
        isBC = true; return 16;

    default:
        throw std::exception("Unsupported DXGI format");
    }
}

inline void VertexIndexBuffersToViews(const D12Resource* vertexBuffer, const D12Resource* indexBuffer, const size_t vertexCount, const size_t vertexStride, const size_t indexCount, D3D12_VERTEX_BUFFER_VIEW& viewV, D3D12_INDEX_BUFFER_VIEW& viewI)
{
    viewV.BufferLocation = vertexBuffer->GetResource()->GetGPUVirtualAddress();
    viewV.SizeInBytes = vertexCount * vertexStride;
    viewV.StrideInBytes = vertexStride;

    viewI.BufferLocation = indexBuffer->GetResource()->GetGPUVirtualAddress();
    viewI.SizeInBytes = indexCount * sizeof(uint32_t);
    viewI.Format = DXGI_FORMAT_R32_UINT;
}

inline D3D12_GRAPHICS_PIPELINE_STATE_DESC CreateGraphicsPipelineDesc(ID3D12RootSignature* rootSig,
                                                                     const D3D12_INPUT_LAYOUT_DESC& ild,
                                                                     bool dsvEnabled = false,
                                                                     uint32_t numRTVs = 1,
                                                                     D3D12_PRIMITIVE_TOPOLOGY_TYPE topologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE)
{
    D3D12_GRAPHICS_PIPELINE_STATE_DESC desc = {};
    desc.InputLayout = ild;
    desc.pRootSignature = rootSig;
    desc.PrimitiveTopologyType = topologyType;

    desc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
    desc.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;

    desc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
    desc.SampleMask = UINT_MAX;
    desc.SampleDesc.Count = 1;

    desc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
    desc.DSVFormat = DXGI_FORMAT_D32_FLOAT;
    desc.DepthStencilState.DepthEnable = dsvEnabled ? TRUE : FALSE;
    desc.DepthStencilState.StencilEnable = dsvEnabled ? TRUE : FALSE;
    desc.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;

    desc.NumRenderTargets = numRTVs;
    for (int i = 0; i < numRTVs; i++)
        desc.RTVFormats[i] = Config::GetRender().RtvFormat;

    return desc;
}

inline D3D12_COMPUTE_PIPELINE_STATE_DESC CreateComputePipelineDesc(ID3D12RootSignature* rootSig)
{
    D3D12_COMPUTE_PIPELINE_STATE_DESC desc = {};
    desc.pRootSignature = rootSig;
    return desc;
}

inline void DispatchOverTexture(ID3D12GraphicsCommandList* cmdList, const uint32_t threadCount, const uint32_t width, const uint32_t height = 0, const uint32_t depth = 0)
{
    const uint32_t groupX = width  == 0 ? 1 : (width  + (threadCount-1)) / threadCount;
    const uint32_t groupY = height == 0 ? 1 : (height + (threadCount-1)) / threadCount;
    const uint32_t groupZ = depth  == 0 ? 1 : (depth  + (threadCount-1)) / threadCount;
    cmdList->Dispatch(groupX, groupY, groupZ);
}

inline CD3DX12_RECT SetViewportScissor(ID3D12GraphicsCommandList* cmdList, const uint32_t width, const uint32_t height, const uint32_t widthOffset = 0)
{
    const CD3DX12_VIEWPORT viewport(static_cast<float>(widthOffset), 0.0f, static_cast<float>(width), static_cast<float>(height));
    const CD3DX12_RECT scissorRect(widthOffset, 0, width + widthOffset, height);

    cmdList->RSSetViewports(1, &viewport);
    cmdList->RSSetScissorRects(1, &scissorRect);

    return scissorRect;
}

inline uint32_t CreateRTV(ID3D12Device* device, Heap* heapRTV, const D12Resource* resource)
{
    D3D12_RENDER_TARGET_VIEW_DESC desc{};
    desc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
    desc.Format = resource->GetDesc().Format;
    desc.Texture2D.MipSlice = 0;
    desc.Texture2D.PlaneSlice = 0;

    const uint32_t heapIdx = heapRTV->GetNextDescriptorIdx(resource->GetName());
    const auto handle = heapRTV->GetDescriptorHandleAtIndex(heapIdx);

    device->CreateRenderTargetView(resource->GetResource(), &desc, handle);

    return heapIdx;
}

inline uint32_t CreateDSV(ID3D12Device* device, Heap* heapDSV, const D12Resource* resource, DXGI_FORMAT format)
{
    D3D12_DEPTH_STENCIL_VIEW_DESC desc{};
    desc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
    desc.Format = format;
    desc.Texture2D.MipSlice = 0;

    const uint32_t heapIdx = heapDSV->GetNextDescriptorIdx(resource->GetName());
    const auto handle = heapDSV->GetDescriptorHandleAtIndex(heapIdx);

    device->CreateDepthStencilView(resource->GetResource(), &desc, handle);

    return heapIdx;
}

#endif