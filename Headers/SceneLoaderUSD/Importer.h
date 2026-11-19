//
// Created by fionaw on 30/05/2026.
//

#ifndef CHERRYSPROUT_IMPORTER_H
#define CHERRYSPROUT_IMPORTER_H

#include <string>
#include <unordered_map>
#include <vector>

#include "Scene/Animation.h"
#include "Scene/Material.h"
#include "Scene/PunctualLight.h"
#include "Scene/Vertex.h"

namespace SceneLoaderUSD
{
    struct ImporterObject
    {
        std::string Name = "Unnamed Object";

        std::vector<Vertex> Vertices;
        std::vector<VertexBoneWeights> BoneWeights;
        std::vector<uint32_t> Indices;

        float M[16];
        int MaterialIndex = -1;
        int SkeletonIndex = -1;
    };

    struct ImporterSkeleton
    {
        uint32_t AnimationClipOffset;
        uint32_t AnimationClipCount;

        std::vector<BoneInfo> BoneInfos;
        std::vector<uint32_t> BoneRemapToSorted;
    };

    struct ImporterContext
    {
        std::vector<ImporterObject> Objects;
        std::vector<Material> Materials;
        std::vector<PunctualLight> PunctualLights;
        std::vector<KeyFrame> KeyFrames;
        std::vector<AnimationClip> AnimationClips;
        std::vector<ImporterSkeleton> Skeletons;
        std::vector<const wchar_t*> TextureFilePaths;
    };

    ImporterContext Import(const char* usdPath, float sceneScale);
}


#endif //CHERRYSPROUT_IMPORTER_H