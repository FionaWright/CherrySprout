//
// Created by fiona on 24/09/2025.
//

#ifndef PT_FILEHELPER_H
#define PT_FILEHELPER_H


class FileHelper
{
public:
    static std::string GetAssetsPath() { return m_assetsPath; }
    static std::string GetShadersPath() { return m_shadersPath; }
    static std::string GetAssetFullPath(const char* assetName);
    static std::string GetAssetTextureFullPath(const char* assetName);
    static std::string GetAssetShaderFullPath(const char* assetName);
    static std::string GetAssetModelFullPath(const char* assetName);

    static std::vector<uint8_t> ReadFileToByteVector(const std::string& filename);
    static HRESULT ReadDataFromFile(const char* filename, byte** data, UINT* size);
    static HRESULT ReadDataFromDDSFile(const char* filename, byte** data, UINT* offset, UINT* size);

private:
    static std::string m_assetsPath, m_shadersPath;
};

#endif //PT_FILEHELPER_H
