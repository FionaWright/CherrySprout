//
// Created by fionaw on 29/05/2026.
//

#ifndef CHERRYSPROUT_SCENE_GPU_H
#define CHERRYSPROUT_SCENE_GPU_H

#include "HWI/D12Resource.h"

struct SceneGPU
{
    D12Resource MegaBufferVertex;
    D12Resource MegaBufferIndex;
    D12Resource MegaBufferInstanceData;
    D12Resource MegaBufferMaterials;
    D12Resource MegaBufferPunctualLights;

    std::vector<D12Resource> SceneTextures;
};

#endif //CHERRYSPROUT_SCENE_GPU_H
