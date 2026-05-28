//
// Created by fionaw on 29/05/2026.
//

#ifndef CHERRYSPROUT_SCENE_H
#define CHERRYSPROUT_SCENE_H

#include "Scene/Vertex.h"
#include "Scene/InstanceData.h"
#include "Scene/Material.h"

struct Object
{
#ifdef _DEBUG
    std::string DebugName = "Unnamed Object";
#endif

    XMFLOAT4x4 M;
    uint32_t MaterialIndex;
};

struct SceneCPU
{
    std::vector<Object> Objects;
    std::vector<std::string> TextureFilepaths;

    std::vector<Vertex> MegaBufferVertex;
    std::vector<uint32_t> MegaBufferIndex;
    std::vector<InstanceData> MegaBufferInstanceData;
    std::vector<Material> MegaBufferMaterials;
    // TODO: MegaBufferPunctualLights
};

struct SceneGPU
{
    D12Resource MegaBufferVertex;
    D12Resource MegaBufferIndex;
    D12Resource MegaBufferInstanceData;
    D12Resource MegaBufferMaterials;
    // TODO: MegaBufferPunctualLights

    D12Resource RTAS;

    // TODO: Textures
};

struct Scene
{
    SceneCPU CPU;
    SceneGPU GPU;
};

#endif //CHERRYSPROUT_SCENE_H
