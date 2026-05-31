//
// Created by fionaw on 29/05/2026.
//

#ifndef CHERRYSPROUT_SCENE_CPU_H
#define CHERRYSPROUT_SCENE_CPU_H

#include <string>
#include <vector>

#include "Scene/Vertex.h"
#include "Scene/InstanceData.h"
#include "Scene/Material.h"

struct Object
{
#ifdef _DEBUG
    std::string DebugName = "Unnamed Object";
#endif

    float M[16];
    uint32_t MaterialIndex;

    uint32_t MegaBufferVertexOffset;
    uint32_t MegaBufferIndexOffset;
    uint32_t MegaBufferIndexCount;
};

struct SceneCPU
{
    // Filled by Scene Loader:
    std::vector<Object> Objects;
    std::vector<std::string> TextureFilepaths;

    std::vector<Vertex> MegaBufferVertex;
    std::vector<uint32_t> MegaBufferIndex;
    std::vector<Material> MegaBufferMaterials;
    // TODO: MegaBufferPunctualLights

    // Filled by RTAS Builder:
    std::vector<InstanceData> MegaBufferInstanceData;
};

#endif //CHERRYSPROUT_SCENE_CPU_H
