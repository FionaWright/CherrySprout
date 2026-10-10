#include "System/pch.h"

#include "System/TextureLoader.h"

#include "HWI/D12Resource.h"
#include "System/FileHelper.h"
#include "Utils/Helper.h"

std::wstring AssetPathToDDS(const wchar_t* path)
{
    const std::filesystem::path p(path);

    if (p.extension() == L".dds")
        return path;

    const std::wstring generic = p.generic_wstring();

    const size_t assetsPos = generic.find(L"/Assets/");
    if (assetsPos == std::string::npos)
        return {};

    std::filesystem::path relative = generic.substr(assetsPos + 8);
    relative.replace_extension(L".dds");

    return FileHelper::GetAssetFullPath(relative.generic_wstring().c_str());
}

D12Resource TextureLoader::LoadTexture2DLDR(ID3D12Device* device, const wchar_t* path, ScratchImage& scratchImage, const D3D12_RESOURCE_FLAGS flags, const bool tryOverloadDDS)
{
    TexMetadata texMetadata{};

    if (tryOverloadDDS)
    {
        const std::wstring ddsPath = AssetPathToDDS(path);
        CherryAssert(std::filesystem::exists(ddsPath));
        CherryAssert(std::filesystem::path(ddsPath).extension().string() == ".dds");

        constexpr DDS_FLAGS ddsFlags = DDS_FLAGS_ALLOW_LARGE_FILES | DDS_FLAGS_IGNORE_MIPS;

        V(LoadFromDDSFile(ddsPath.c_str(), ddsFlags, &texMetadata, scratchImage));
    }
    else
    {
        V(LoadFromWICFile(path, WIC_FLAGS_NONE, &texMetadata, scratchImage));
    }

    DXGI_FORMAT format = texMetadata.format;
    if (format == DXGI_FORMAT_R8G8B8A8_UNORM_SRGB)
        format = DXGI_FORMAT_R8G8B8A8_UNORM;

    const char* name = _strdup(wstringToString(path).c_str()); // TODO: Memory leak

    D12Resource texture;
    texture.Init_Tex2D(name, device, texMetadata.width, texMetadata.height, texMetadata.depth, format, flags, D3D12_RESOURCE_STATE_COMMON, nullptr, true);
    return texture;
}

D12Resource TextureLoader::LoadTexture2DHDR(ID3D12Device* device, const wchar_t* path, ScratchImage& scratchImage, const D3D12_RESOURCE_FLAGS flags)
{
    CherryAssert(std::filesystem::exists(path) && std::filesystem::path(path).extension().string() == ".hdr");

    TexMetadata texMetadata{};
    V(LoadFromHDRFile(path, &texMetadata, scratchImage));

    const char* name = _strdup(wstringToString(path).c_str()); // TODO: Memory leak

    D12Resource texture;
    texture.Init_Tex2D(name, device, texMetadata.width, texMetadata.height, texMetadata.depth, texMetadata.format, flags, D3D12_RESOURCE_STATE_COPY_DEST);
    return texture;
}

void TextureLoader::UploadTexture(ID3D12GraphicsCommandList* cmdList, UploadHeap* uploadHeap, const ScratchImage& scratchImage, D12Resource* texture)
{
    texture->UploadTexture(cmdList, uploadHeap, scratchImage.GetPixels(), scratchImage.GetPixelsSize(), scratchImage.GetImage(0,0,0)->rowPitch);
}
