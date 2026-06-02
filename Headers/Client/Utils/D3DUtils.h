#ifndef H_D3D_UTILS_H
#define H_D3D_UTILS_H

#include <d3d12.h>

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

#endif