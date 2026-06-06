#ifndef H_TEXTURE_LOADER_H
#define H_TEXTURE_LOADER_H

class UploadHeap;
class D12Resource;

class TextureLoader
{
public:
    static D12Resource LoadTexture2DLDR(ID3D12Device* device, const char* path, ScratchImage& scratchImage, D3D12_RESOURCE_FLAGS flags = D3D12_RESOURCE_FLAG_NONE);
    static D12Resource LoadTexture2DHDR(ID3D12Device* device, const char* path, ScratchImage& scratchImage, D3D12_RESOURCE_FLAGS flags = D3D12_RESOURCE_FLAG_NONE);

    static void UploadTexture(ID3D12GraphicsCommandList* cmdList, UploadHeap* uploadHeap, const ScratchImage& scratchImage, D12Resource* texture);
};

#endif