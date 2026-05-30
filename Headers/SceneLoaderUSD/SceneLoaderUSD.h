#ifndef H_SCENE_LOADER_H
#define H_SCENE_LOADER_H

#include "Scene/SceneCPU.h"

namespace SceneLoaderUSD
{
    __declspec(dllexport) void LoadUSD(const char* filepath, SceneCPU* scene);
}

#endif