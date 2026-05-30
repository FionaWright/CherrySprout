
#include "SceneLoaderUSD.h"

void SceneLoaderUSD::LoadUSD(const char* filepath, SceneCPU* scene)
{
    scene->TextureFilepaths.emplace_back(filepath);
}