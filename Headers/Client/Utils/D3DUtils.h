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

#endif