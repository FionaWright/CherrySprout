//
// Created by fiona on 24/09/2025.
//

#include "System/pch.h"
#include "System/FileHelper.h"
#include <wrl/wrappers/corewrappers.h>

#include "Utils/Helper.h"

std::string FileHelper::m_assetsPath = std::string(CHERRYSPROUT_ASSETS_DIR) + "/";
std::string FileHelper::m_shadersPath = std::string(CHERRYSPROUT_SHADERS_DIR) + "/";

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