//
// Created by fiona on 24/09/2025.
//

#include "System/pch.h"
#include "System/FileHelper.h"
#include <wrl/wrappers/corewrappers.h>

#include "Utils/Helper.h"

std::string FileHelper::m_assetsPath = std::string(CHERRYSPROUT_ASSETS_DIR) + "/";
std::wstring FileHelper::m_assetsPathW = std::wstring(CHERRYSPROUT_ASSETS_DIR_W) + L"/";
std::string FileHelper::m_shadersPath = std::string(CHERRYSPROUT_SHADERS_DIR) + "/";

std::string FileHelper::GetAssetFullPath(const char* assetName)
{
    return m_assetsPath + assetName;
}

std::wstring FileHelper::GetAssetFullPath(const wchar_t* assetName)
{
    return m_assetsPathW + assetName;
}

std::string FileHelper::GetAssetTextureFullPath(const char* assetName)
{
    return m_assetsPath + "Textures/" + assetName;
}

std::wstring FileHelper::GetAssetTextureFullPath(const wchar_t* assetName)
{
    return m_assetsPathW + L"Textures/" + assetName;
}

std::string FileHelper::GetAssetShaderFullPath(const char* assetName)
{
    return m_shadersPath + assetName;
}

std::string FileHelper::GetAssetModelFullPath(const char* assetName)
{
    return m_assetsPath + "Models/" + assetName;
}