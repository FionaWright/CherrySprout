#ifndef H_SCENE_LOADER_H
#define H_SCENE_LOADER_H

#include "Scene/SceneCPU.h"

extern "C" __declspec(dllexport)
void LoadUSD(const char* usdPath, float sceneScale, SceneCPU* scene);

#endif