#include "System/pch.h"

#include "Debug/Snapshotter.h"

#include <wincodec.h>

#include "HWI/D12Resource.h"
#include "HWI/D3D.h"
#include "Utils/Helper.h"

void CopyTexToBuffer(ID3D12Device* device, ID3D12GraphicsCommandList* cmdList, const D12Resource* tex, const D12Resource* buff)
{
    D3D12_TEXTURE_COPY_LOCATION srcLocation = {};
    srcLocation.pResource = tex->GetResource();
    srcLocation.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
    srcLocation.SubresourceIndex = 0;

    UINT height;
    UINT64 rowPitch, slicePitch;
    D3D12_PLACED_SUBRESOURCE_FOOTPRINT footprint;
    {
        const D3D12_RESOURCE_DESC desc = tex->GetDesc();
        device->GetCopyableFootprints(&desc, 0, 1, 0,
            &footprint, &height, &rowPitch, &slicePitch);
    }

    D3D12_TEXTURE_COPY_LOCATION dstLocation = {};
    dstLocation.pResource = buff->GetResource();
    dstLocation.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
    dstLocation.PlacedFootprint = footprint;

    cmdList->CopyTextureRegion(&dstLocation, 0, 0, 0, &srcLocation, nullptr);
}
void Snapshotter::ResourceToSnapshot(D3D* d3d, D12Resource* d12Resource, uint8_t*& data, size_t& dataSize)
{
    D12Resource readbackBuffer;

    dataSize = d12Resource->GetIntermediateSize();
    readbackBuffer.Init_Buffer("Snapshotter Readback Buffer", d3d->GetDevice(), dataSize, D3D12_RESOURCE_FLAG_NONE, true, D3D12_RESOURCE_STATE_COPY_DEST);

    d3d->Flush();
    const auto cmdListPtr = d3d->GetAvailableCmdList(D3D12_COMMAND_LIST_TYPE_DIRECT);
    const auto cmdList = cmdListPtr.Get();
    {
        d12Resource->Transition(cmdList, D3D12_RESOURCE_STATE_COPY_SOURCE);
        CopyTexToBuffer(d3d->GetDevice(), cmdList, d12Resource, &readbackBuffer);
    }
    V(cmdList->Close());
    d3d->ExecuteCommandList(cmdList);
    d3d->Flush();

    data = new uint8_t[dataSize];
    readbackBuffer.Readback(data);
}

const Image* PackData(const D3D* d3d, uint8_t* data, const D12Resource* d12Resource, ScratchImage& scratch)
{
    UINT numRows;
    UINT64 rowPitch, slicePitch;
    D3D12_PLACED_SUBRESOURCE_FOOTPRINT footprint;
    {
        const D3D12_RESOURCE_DESC desc = d12Resource->GetDesc();
        d3d->GetDevice()->GetCopyableFootprints(&desc, 0, 1, 0,
            &footprint, &numRows, &rowPitch, &slicePitch);
    }

    Image image = {};
    image.format = d12Resource->GetDesc().Format;
    image.pixels = data;
    image.width = d12Resource->GetDesc().Width;
    image.height = d12Resource->GetDesc().Height;
    image.rowPitch = footprint.Footprint.RowPitch;
    image.slicePitch = footprint.Footprint.RowPitch * image.height;

    V(scratch.Initialize2D(
        image.format,
        image.width,
        image.height,
        1,
        1));
    const Image* dst = scratch.GetImage(0,0,0);
    for (size_t y = 0; y < image.height; ++y)
    {
        memcpy(
            dst->pixels + y * dst->rowPitch,
            image.pixels + y * image.rowPitch,
            std::min(dst->rowPitch, image.rowPitch));
    }
    return dst;
}

void Snapshotter::SnapshotToRgba8(const D3D* d3d, uint8_t* data, const D12Resource* d12Resource, uint8_t*& dataRgba8)
{
    ScratchImage scratch;
    const Image* image = PackData(d3d, data, d12Resource, scratch);

    ScratchImage converted;
    V(Convert(*image, DXGI_FORMAT_R8G8B8A8_UNORM, TEX_FILTER_DEFAULT, TEX_THRESHOLD_DEFAULT, converted));
    dataRgba8 = converted.GetPixels();
}

bool FormatIsHDR(const DXGI_FORMAT format)
{
    switch (format)
    {
    case DXGI_FORMAT_R16G16B16A16_FLOAT:
    case DXGI_FORMAT_R32G32B32A32_FLOAT:
    case DXGI_FORMAT_R32G32B32_FLOAT:
    case DXGI_FORMAT_R11G11B10_FLOAT:
    case DXGI_FORMAT_R16_FLOAT:
    case DXGI_FORMAT_R32_FLOAT:
        return true;

    default:
        return false;
    }
}

void Snapshotter::SnapshotToFile(const D3D* d3d, uint8_t* data, const D12Resource* d12Resource, const char* fileName)
{
    ScratchImage scratch;
    const Image* image = PackData(d3d, data, d12Resource, scratch);

    const bool isHDR = FormatIsHDR(d12Resource->GetDesc().Format);

    const std::string fileNameWithExt = std::string(fileName) + (isHDR ? ".hdr" : ".tga");
    const std::wstring fileNameW = stringToWString(fileNameWithExt);

    std::cout << "Saved snapshot to: " << fileNameWithExt << std::endl;

    if (isHDR)
        V(SaveToHDRFile(*image, fileNameW.c_str()));
    else
        V(SaveToTGAFile(*image, TGA_FLAGS_ALLOW_ALL_ZERO_ALPHA | TGA_FLAGS_DEFAULT_SRGB, fileNameW.c_str()));
}

void Snapshotter::Rgba8SnapshotToClipboard(const D3D* d3d, uint8_t* data, const D12Resource* d12Resource)
{
    if (!data || !d12Resource)
        return;

    ScratchImage scratch;
    const Image* image = PackData(d3d, data, d12Resource, scratch);

    // Disable transparency
    uint8_t* pixels = image->pixels;
    for (size_t i = 0; i < image->width * image->height; ++i)
    {
        pixels[i * 4 + 3] = 255;
    }

    // PNG
    HGLOBAL hMemPng = nullptr;
    {
        Blob pngBlob;
        V(SaveToWICMemory(
            *image,
            WIC_FLAGS_FORCE_SRGB,
            GUID_ContainerFormatPng,
            pngBlob));

        hMemPng = GlobalAlloc(
            GMEM_MOVEABLE,
            pngBlob.GetBufferSize());
        if (!hMemPng)
            return;

        void* dst = GlobalLock(hMemPng);
        if (!dst)
        {
            GlobalFree(hMemPng);
            return;
        }

        memcpy(
            dst,
            pngBlob.GetBufferPointer(),
            pngBlob.GetBufferSize());

        GlobalUnlock(hMemPng);
    }

    HGLOBAL hMemDibv5 = nullptr;
    {
        const DWORD imageSize = image->slicePitch;
        const SIZE_T totalSize =
            sizeof(BITMAPV5HEADER) + imageSize;

        hMemDibv5 = GlobalAlloc(GMEM_MOVEABLE, totalSize);
        if (!hMemDibv5)
            return;

        auto* p = static_cast<uint8_t*>(GlobalLock(hMemDibv5));
        if (!p)
        {
            GlobalFree(hMemDibv5);
            return;
        }

        auto* hdr = reinterpret_cast<BITMAPV5HEADER*>(p);

        ZeroMemory(hdr, sizeof(BITMAPV5HEADER));

        hdr->bV5Size = sizeof(BITMAPV5HEADER);
        hdr->bV5Width = static_cast<LONG>(image->width);
        hdr->bV5SizeImage = imageSize;

        // negative = top-down bitmap
        hdr->bV5Height = -static_cast<LONG>(image->height);

        hdr->bV5Planes = 1;
        hdr->bV5BitCount = 32;
        hdr->bV5Compression = BI_BITFIELDS;

        hdr->bV5RedMask   = 0x000000FF;
        hdr->bV5GreenMask = 0x0000FF00;
        hdr->bV5BlueMask  = 0x00FF0000;
        hdr->bV5AlphaMask = 0xFF000000;

        hdr->bV5CSType = LCS_sRGB;

        uint8_t* dstPixels = p + sizeof(BITMAPV5HEADER);

        memcpy(dstPixels, image->pixels, imageSize);

        GlobalUnlock(hMemDibv5);
    }

    if (!OpenClipboard(nullptr))
    {
        GlobalFree(hMemDibv5);
        GlobalFree(hMemPng);
        return;
    }

    EmptyClipboard();

    if (!SetClipboardData(CF_DIBV5, hMemDibv5))
    {
        GlobalFree(hMemDibv5);
    }

    const UINT pngFormat = RegisterClipboardFormatA("PNG");
    assert(pngFormat != 0);

    if (!SetClipboardData(pngFormat, hMemPng))
    {
        GlobalFree(hMemPng);
    }

    // Clipboard owns hMem after success.
    CloseClipboard();
}
