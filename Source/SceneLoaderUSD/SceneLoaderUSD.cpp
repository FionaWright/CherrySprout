
#include "SceneLoaderUSD.h"

#include <iostream>
#include <ostream>
#include <filesystem>

#include "Importer.h"
#include "Processor.h"

void LoadUSD(const char* usdPath, float sceneScale, SceneCPU* scene)
{
    std::cout << "Scene Loader USD: " << usdPath << std::endl;

    if (!std::filesystem::exists(usdPath))
    {
        std::cout << "USD file does not exist" << std::endl;
        return;
    }

    if (!std::filesystem::path(usdPath).extension().string().starts_with(".usd"))
    {
        std::cout << "Not a path to a USD file!" << std::endl;
        return;
    }

    const auto importerContext = SceneLoaderUSD::Import(usdPath, sceneScale);
    SceneLoaderUSD::Process(importerContext, scene);
}

void FreeScene(SceneCPU* scene)
{
    // TODO
}
