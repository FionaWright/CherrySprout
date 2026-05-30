
#include "SceneLoaderUSD.h"

#include <iostream>
#include <ostream>
#include <filesystem>

#include "Importer.h"
#include "Processor.h"

void SceneLoaderUSD::LoadUSD(const char* usdPath, SceneCPU* scene)
{
    std::cout << "Scene Loader USD: " << usdPath << std::endl;

    if (!std::filesystem::exists(usdPath))
        throw std::runtime_error("USD file does not exist");

    if (!std::filesystem::path(usdPath).extension().string().starts_with(".usd"))
        throw std::runtime_error("Not a path to a USD file!");

    const auto importerContext = Import(usdPath);
    Process(importerContext, scene);
}
