
#include "SceneLoaderUSD.h"

#include <iostream>
#include <ostream>
#include <filesystem>

#include "Importer.h"
#include "Processor.h"

void LoadUSD(const char* usdPath, const float sceneScale, SceneCPU* scene)
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
    std::cout << "Freeing Scene..." << std::endl;

    for (int i = 0; i < scene->ObjectCount; i++)
    {
#if !NDEBUG
        free(scene->Objects[i].DebugName);
        scene->Objects[i].DebugName = nullptr;
#endif
    }

    free(scene->Objects);
    scene->Objects = nullptr;

    free(scene->TextureFilepaths);
    scene->TextureFilepaths = nullptr;

    free(scene->MegaBufferVertex);
    scene->MegaBufferVertex = nullptr;

    free(scene->MegaBufferIndex);
    scene->MegaBufferIndex = nullptr;

    free(scene->MegaBufferMaterials);
    scene->MegaBufferMaterials = nullptr;

    free(scene->MegaBufferPunctualLights);
    scene->MegaBufferPunctualLights = nullptr;
}
