//
// Created by fionaw on 29/05/2026.
//

#ifndef CHERRYSPROUT_SCENE_CPU_H
#define CHERRYSPROUT_SCENE_CPU_H

#include "Scene/Vertex.h"
#include "Scene/Material.h"
#include "Scene/PunctualLight.h"

struct Object
{
#if _DEBUG
    const char* DebugName = "Unnamed Object";
#endif

    float M[16];
    uint32_t MaterialIndex;

    uint32_t MegaBufferVertexOffset;
    uint32_t MegaBufferVertexCount;
    uint32_t MegaBufferIndexOffset;
    uint32_t MegaBufferIndexCount;
};

struct SceneCPU
{
    // Filled by Scene Loader:
    Object* Objects = nullptr;
    size_t ObjectCount = 0;

    char** TextureFilepaths = nullptr;
    size_t TextureFilepathCount = 0;

    Vertex* MegaBufferVertex = nullptr;
    size_t MegaBufferVertexCount = 0;

    uint32_t* MegaBufferIndex = nullptr;
    size_t MegaBufferIndexCount = 0;

    Material* MegaBufferMaterials = nullptr;
    size_t MegaBufferMaterialsCount = 0;

    PunctualLight* MegaBufferPunctualLights = nullptr;
    size_t MegaBufferPunctualLightsCount = 0;
};

#endif //CHERRYSPROUT_SCENE_CPU_H
