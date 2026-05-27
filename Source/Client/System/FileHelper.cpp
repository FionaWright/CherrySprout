//
// Created by fiona on 24/09/2025.
//

#include "System/pch.h"
#include "System/FileHelper.h"
#include <wrl/wrappers/corewrappers.h>

#include "Utils/Helper.h"

std::string FileHelper::m_assetsPath, FileHelper::m_shadersPath;

void FileHelper::Init()
{
    m_assetsPath = std::string(CHERRYSPROUT_ASSETS_DIR) + "/";
    m_shadersPath = std::string(CHERRYSPROUT_SHADERS_DIR) + "/";
}

std::string FileHelper::GetAssetFullPath(const char* assetName)
{
    return m_assetsPath + assetName;
}

std::string FileHelper::GetAssetTextureFullPath(const char* assetName)
{
    return m_assetsPath + "Textures/" + assetName;
}

std::string FileHelper::GetAssetShaderFullPath(const char* assetName)
{
    return m_shadersPath + assetName;
}

std::string FileHelper::GetAssetModelFullPath(const char* assetName)
{
    return m_assetsPath + "Models/" + assetName;
}

std::vector<uint8_t> FileHelper::ReadFileToByteVector(const std::string& filename)
{
    std::ifstream file(filename, std::ios::binary | std::ios::ate);
    if (!file) throw std::runtime_error("Failed to open file");
    const auto size = static_cast<size_t>(file.tellg());
    std::vector<uint8_t> data(size);
    file.seekg(0);
    file.read(reinterpret_cast<char*>(data.data()), size);
    return data;
}

inline HRESULT FileHelper::ReadDataFromFile(const char* filename, byte** data, UINT* size)
{
#if WINVER >= _WIN32_WINNT_WIN8
    CREATEFILE2_EXTENDED_PARAMETERS extendedParams = {};
    extendedParams.dwSize = sizeof(CREATEFILE2_EXTENDED_PARAMETERS);
    extendedParams.dwFileAttributes = FILE_ATTRIBUTE_NORMAL;
    extendedParams.dwFileFlags = FILE_FLAG_SEQUENTIAL_SCAN;
    extendedParams.dwSecurityQosFlags = SECURITY_ANONYMOUS;
    extendedParams.lpSecurityAttributes = nullptr;
    extendedParams.hTemplateFile = nullptr;

    std::wstring filenameW = stringToWString(filename);
    Microsoft::WRL::Wrappers::FileHandle file(CreateFile2(filenameW.c_str(), GENERIC_READ, FILE_SHARE_READ, OPEN_EXISTING, &extendedParams));
#else
    Wrappers::FileHandle file(CreateFile(filename, GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL | FILE_FLAG_SEQUENTIAL_SCAN | SECURITY_SQOS_PRESENT | SECURITY_ANONYMOUS, nullptr));
#endif
    if (file.Get() == INVALID_HANDLE_VALUE)
    {
        throw std::exception();
    }

    FILE_STANDARD_INFO fileInfo = {};
    if (!GetFileInformationByHandleEx(file.Get(), FileStandardInfo, &fileInfo, sizeof(fileInfo)))
    {
        throw std::exception();
    }

    if (fileInfo.EndOfFile.HighPart != 0)
    {
        throw std::exception();
    }

    *data = reinterpret_cast<byte*>(malloc(fileInfo.EndOfFile.LowPart));
    *size = fileInfo.EndOfFile.LowPart;

    if (!ReadFile(file.Get(), *data, fileInfo.EndOfFile.LowPart, nullptr, nullptr))
    {
        throw std::exception();
    }

    return S_OK;
}

inline HRESULT FileHelper::ReadDataFromDDSFile(const char* filename, byte** data, UINT* offset, UINT* size)
{
    if (FAILED(ReadDataFromFile(filename, data, size)))
    {
        return E_FAIL;
    }

    // DDS files always start with the same magic number.
    static const UINT DDS_MAGIC = 0x20534444;
    UINT magicNumber = *reinterpret_cast<const UINT*>(*data);
    if (magicNumber != DDS_MAGIC)
    {
        return E_FAIL;
    }

    struct DDS_PIXELFORMAT
    {
        UINT size;
        UINT flags;
        UINT fourCC;
        UINT rgbBitCount;
        UINT rBitMask;
        UINT gBitMask;
        UINT bBitMask;
        UINT aBitMask;
    };

    struct DDS_HEADER
    {
        UINT size;
        UINT flags;
        UINT height;
        UINT width;
        UINT pitchOrLinearSize;
        UINT depth;
        UINT mipMapCount;
        UINT reserved1[11];
        DDS_PIXELFORMAT ddsPixelFormat;
        UINT caps;
        UINT caps2;
        UINT caps3;
        UINT caps4;
        UINT reserved2;
    };

    auto ddsHeader = reinterpret_cast<const DDS_HEADER*>(*data + sizeof(UINT));
    if (ddsHeader->size != sizeof(DDS_HEADER) || ddsHeader->ddsPixelFormat.size != sizeof(DDS_PIXELFORMAT))
    {
        return E_FAIL;
    }

    const ptrdiff_t ddsDataOffset = sizeof(UINT) + sizeof(DDS_HEADER);
    *offset = ddsDataOffset;
    *size = *size - ddsDataOffset;

    return S_OK;
}