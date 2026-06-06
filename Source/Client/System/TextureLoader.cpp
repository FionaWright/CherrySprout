#include "System/pch.h"

#include "System/TextureLoader.h"

#include "HWI/D12Resource.h"
#include "System/FileHelper.h"
#include "Utils/Helper.h"

std::string AssetPathToDDS(const char* path)
{
    std::filesystem::path p(path);

    const std::string generic = p.generic_string();

    const size_t assetsPos = generic.find("/Assets/");
    if (assetsPos == std::string::npos)
        return {};

    std::filesystem::path relative = generic.substr(assetsPos + 8);
    relative.replace_extension(".dds");

    return FileHelper::GetAssetFullPath(relative.generic_string().c_str());
}

D12Resource TextureLoader::LoadTexture2DLDR(ID3D12Device* device, const char* path, ScratchImage& scratchImage, const D3D12_RESOURCE_FLAGS flags)
{
    const std::string ddsPath = AssetPathToDDS(path);

    CherryAssert(std::filesystem::exists(ddsPath) && std::filesystem::path(ddsPath).extension().string() == ".dds");

    const std::wstring fullPathW = stringToWString(ddsPath);

    constexpr DDS_FLAGS ddsFlags = DDS_FLAGS_ALLOW_LARGE_FILES | DDS_FLAGS_IGNORE_MIPS;

    TexMetadata texMetadata;
    V(LoadFromDDSFile(fullPathW.c_str(), ddsFlags, &texMetadata, scratchImage));

    D12Resource texture;
    texture.Init_Tex2D(path, device, texMetadata.width, texMetadata.height, texMetadata.depth, texMetadata.format, flags, D3D12_RESOURCE_STATE_COMMON);
    return texture;
}

D12Resource TextureLoader::LoadTexture2DHDR(ID3D12Device* device, const char* path, ScratchImage& scratchImage, const D3D12_RESOURCE_FLAGS flags)
{
    CherryAssert(std::filesystem::exists(path) && std::filesystem::path(path).extension().string() == ".hdr");

    const std::wstring fullPathW = stringToWString(path);

    TexMetadata texMetadata;
    V(LoadFromHDRFile(fullPathW.c_str(), &texMetadata, scratchImage));

    D12Resource texture;
    texture.Init_Tex2D(path, device, texMetadata.width, texMetadata.height, texMetadata.depth, texMetadata.format, flags, D3D12_RESOURCE_STATE_COPY_DEST);
    return texture;
}

void TextureLoader::UploadTexture(ID3D12GraphicsCommandList* cmdList, UploadHeap* uploadHeap, const ScratchImage& scratchImage, D12Resource* texture)
{
    texture->UploadTexture(cmdList, uploadHeap, scratchImage.GetPixels(), scratchImage.GetPixelsSize(), scratchImage.GetImage(0,0,0)->rowPitch);
}
