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
    static std::wstring GetAssetFullPath(const wchar_t* assetName);
    static std::string GetAssetTextureFullPath(const char* assetName);
    static std::wstring GetAssetTextureFullPath(const wchar_t* assetName);
    static std::string GetAssetShaderFullPath(const char* assetName);
    static std::string GetAssetModelFullPath(const char* assetName);

private:
    static std::string m_assetsPath, m_shadersPath;
    static std::wstring m_assetsPathW;
};

#endif //PT_FILEHELPER_H
