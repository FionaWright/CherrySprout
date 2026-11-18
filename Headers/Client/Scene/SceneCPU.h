//
// Created by fionaw on 29/05/2026.
//

#ifndef CHERRYSPROUT_SCENE_CPU_H
#define CHERRYSPROUT_SCENE_CPU_H

#include "Scene/Vertex.h"
#include "Scene/Material.h"
#include "Scene/Animation.h"
#include "Scene/PunctualLight.h"

struct Object
{
#if !NDEBUG
    char* DebugName = nullptr;
#endif

    XMMATRIX M;
    uint32_t MaterialIndex{};

    uint32_t MegaBufferVertexOffset{};
    uint32_t VertexCount{};

    uint32_t MegaBufferIndexOffset{};
    uint32_t IndexCount{};

    uint32_t MegaBufferAnimationClipOffset{};
    uint32_t AnimationClipCount{};

    uint32_t MegaBufferBoneOffset{};
    uint32_t BoneCount{};
};

struct SceneCPU
{
    // Filled by Scene Loader:
    Object* Objects = nullptr;
    size_t ObjectCount = 0;

    wchar_t** TextureFilepaths = nullptr;
    size_t TextureFilepathCount = 0;

    Vertex* MegaBufferVertex = nullptr;
    size_t MegaBufferVertexCount = 0;

    uint32_t* MegaBufferIndex = nullptr;
    size_t MegaBufferIndexCount = 0;

    Material* MegaBufferMaterials = nullptr;
    size_t MegaBufferMaterialsCount = 0;

    PunctualLight* MegaBufferPunctualLights = nullptr;
    size_t MegaBufferPunctualLightsCount = 0;

    // ============= Animation =============

    AnimationClip* MegaBufferAnimationClips = nullptr;
    size_t MegaBufferAnimationClipsCount = 0;

    KeyFrame* MegaBufferKeyFrames = nullptr;
    size_t MegaBufferKeyFramesCount = 0;

    VertexBoneWeights* MegaBufferVertexBoneWeights = nullptr;
    // Note: MegaBufferVertexBoneWeightsCount == MegaBufferVertexCount

    BoneInfo* MegaBufferBoneInfos = nullptr;
    size_t MegaBufferBonesCount = 0;

    // =====================================
};

#endif //CHERRYSPROUT_SCENE_CPU_H
