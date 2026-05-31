//
// Created by fionaw on 31/05/2026.
//

#include "System/pch.h"
#include "Scene/SceneManager.h"

typedef void (*LoadUSDFunc)(const char* usdPath, SceneCPU* scene);

void SceneManager::LoadScene(const char* filepath)
{
    const bool isUSD = std::filesystem::path(filepath).extension().string().starts_with(".usd");
    if (!isUSD)
        throw std::runtime_error("Non-USD scenes not supported yet!");

    const HMODULE dll = LoadLibraryA("SceneLoaderUSD.dll");
    if (!dll)
    {
        std::cerr << "Failed to load DLL\n";
        return;
    }

    const LoadUSDFunc LoadUSD =
        reinterpret_cast<LoadUSDFunc>(
            GetProcAddress(dll, "LoadUSD"));

    if (!LoadUSD)
    {
        std::cerr << "Failed to find LoadUSD\n";
        FreeLibrary(dll);
        return;
    }

    LoadUSD(filepath, &m_scene.CPU);

    FreeLibrary(dll);
}
