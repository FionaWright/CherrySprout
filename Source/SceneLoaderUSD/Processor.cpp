//
// Created by fionaw on 30/05/2026.
//

#include "Processor.h"

#include <filesystem>
#include <iostream>
#include <ostream>

#include <windows.h>
#include "Utils/CppUtils.h"

void SceneLoaderUSD::Process(const char* usdPath, const ImporterContext& importerContext, SceneCPU* scene)
{
    std::cout << "Processing scene..." << std::endl;

    *scene = {};

    // Materials
    {
        scene->MegaBufferMaterials = new Material[importerContext.Materials.size()];
        scene->MegaBufferMaterialsCount = importerContext.Materials.size();
        memcpy(scene->MegaBufferMaterials, importerContext.Materials.data(), scene->MegaBufferMaterialsCount * sizeof(Material));
    }

    // Textures
    {
        const std::wstring sceneDir = std::filesystem::path(usdPath).parent_path().wstring();

        scene->TextureFilepathCount = importerContext.TextureFilePaths.size();
        scene->TextureFilepaths = static_cast<wchar_t**>(malloc(scene->TextureFilepathCount * sizeof(wchar_t*)));
        for (size_t i = 0; i < scene->TextureFilepathCount; i++)
        {
            std::wstring path = importerContext.TextureFilePaths[i];
            if (path.starts_with('.'))
                path = path.replace(0, 1, sceneDir);

            scene->TextureFilepaths[i] = _wcsdup(path.c_str());
        }
    }

    size_t totalVertexCount = 0;
    size_t totalIndexCount = 0;
    for (const auto & object : importerContext.Objects)
    {
        totalVertexCount += object.Vertices.size();
        totalIndexCount += object.Indices.size();
    }
    scene->MegaBufferVertex = new Vertex[totalVertexCount];
    scene->MegaBufferIndex = new uint32_t[totalIndexCount];

    bool animationInScene = false;
    for (const auto& impObj : importerContext.Objects)
    {
        if (impObj.BoneWeights.size() > 0)
        {
            animationInScene = true;
            break;
        }
    }

    uint32_t totalBoneCount = 0;
    for (size_t i = 0; i < importerContext.Skeletons.size(); i++)
    {
        totalBoneCount += importerContext.Skeletons[i].BoneInfos.size();
    }
    animationInScene &= totalBoneCount > 0;

    std::vector<uint32_t> skeletonBoneBufferOffsets;
    if (animationInScene)
    {
        scene->IsAnimationInScene = true;

        scene->MegaBufferVertexBoneWeights = new VertexBoneWeights[totalVertexCount];
        scene->MegaBufferBoneInfos = new BoneInfo[totalBoneCount];

        skeletonBoneBufferOffsets.resize(importerContext.Skeletons.size());

        for (size_t i = 0; i < importerContext.Skeletons.size(); i++)
        {
            const uint32_t boneOffset = scene->MegaBufferBonesCount;

            scene->MegaBufferBonesCount += importerContext.Skeletons[i].BoneInfos.size();

            BoneInfo* addr = scene->MegaBufferBoneInfos + boneOffset;
            memcpy(addr, importerContext.Skeletons[i].BoneInfos.data(), importerContext.Skeletons[i].BoneInfos.size() * sizeof(BoneInfo));

            skeletonBoneBufferOffsets[i] = boneOffset;
        }
    }
    else
    {
        scene->MegaBufferVertexBoneWeights = new VertexBoneWeights[1];
        scene->MegaBufferBoneInfos = new BoneInfo[1];
        scene->MegaBufferBonesCount = 1;
    }

    scene->ObjectCount = importerContext.Objects.size();
    scene->Objects = new Object[scene->ObjectCount];
    for (size_t i = 0; i < scene->ObjectCount; i++)
    {
        const ImporterObject& impObj = importerContext.Objects[i];
        Object obj{};

#if !NDEBUG
        obj.DebugName = _strdup(impObj.Name.c_str());
#endif

        obj.MegaBufferVertexOffset  = static_cast<uint32_t>(scene->MegaBufferVertexCount);
        obj.MegaBufferIndexOffset   = static_cast<uint32_t>(scene->MegaBufferIndexCount);
        obj.VertexCount   = static_cast<uint32_t>(impObj.Vertices.size());
        obj.IndexCount    = static_cast<uint32_t>(impObj.Indices.size());

        Vertex* vAddr = scene->MegaBufferVertex + obj.MegaBufferVertexOffset;
        uint32_t* iAddr = scene->MegaBufferIndex + obj.MegaBufferIndexOffset;
        memcpy(vAddr, impObj.Vertices.data(), impObj.Vertices.size() * sizeof(Vertex));
        memcpy(iAddr, impObj.Indices.data(), impObj.Indices.size() * sizeof(uint32_t));
        scene->MegaBufferVertexCount += impObj.Vertices.size();
        scene->MegaBufferIndexCount += impObj.Indices.size();

        if (animationInScene && impObj.SkeletonIndex != -1)
        {
            const ImporterSkeleton& impSkeleton = importerContext.Skeletons[impObj.SkeletonIndex];

            std::vector<VertexBoneWeights> boneWeights = impObj.BoneWeights;

            assert(impObj.BoneWeights.size() == impObj.Vertices.size());

            // Remap indices
            for (VertexBoneWeights& weights : boneWeights)
            {
                for (uint32_t ind = 0; ind < MAX_BONE_WEIGHTS_PER_VERTEX; ind++)
                {
                    const uint32_t boneIdx = weights.BoneIndices[ind];
                    if (boneIdx == UINT_MAX)
                        break;

                    weights.BoneIndices[ind] = impSkeleton.BoneRemapToSorted[boneIdx];
                }
            }

            VertexBoneWeights* vbwAddr = scene->MegaBufferVertexBoneWeights + obj.MegaBufferVertexOffset;
            memcpy(vbwAddr, boneWeights.data(), boneWeights.size() * sizeof(VertexBoneWeights));
            for (size_t bw = boneWeights.size(); bw < impObj.Vertices.size(); bw++)
            {
                vbwAddr[bw].BoneIndices[0] = UINT_MAX;
                vbwAddr[bw].BoneWeights[0] = -1;
            }

            obj.AnimationClipCount = impSkeleton.AnimationClipCount;
            obj.MegaBufferAnimationClipOffset = impSkeleton.AnimationClipOffset;
            obj.MegaBufferBoneOffset = skeletonBoneBufferOffsets[impObj.SkeletonIndex];
            obj.BoneCount = impSkeleton.BoneInfos.size();

            obj.SelectedAnimationClipIdx = 0;
        }
        else
            obj.SelectedAnimationClipIdx = -1;

        obj.M = DirectX::XMMatrixSet(
            impObj.M[0], impObj.M[1], impObj.M[2], impObj.M[3],
            impObj.M[4], impObj.M[5], impObj.M[6], impObj.M[7],
            impObj.M[8], impObj.M[9], impObj.M[10], impObj.M[11],
            impObj.M[12], impObj.M[13], impObj.M[14], impObj.M[15]
        );

        obj.MaterialIndex = impObj.MaterialIndex;

        scene->Objects[i] = obj;
    }

    assert(totalVertexCount == scene->MegaBufferVertexCount);
    assert(totalIndexCount == scene->MegaBufferIndexCount);

    if (importerContext.PunctualLights.empty())
    {
        scene->MegaBufferPunctualLightsCount = 1;
        scene->MegaBufferPunctualLights = new PunctualLight[1];
        scene->MegaBufferPunctualLights[0] = PunctualLight();
        scene->MegaBufferPunctualLights[0].Intensity = 0.0f;
        std::cout << "No punctual lights in scene, added dummy" << std::endl;
    }
    else
    {
        scene->MegaBufferPunctualLightsCount = importerContext.PunctualLights.size();
        scene->MegaBufferPunctualLights = new PunctualLight[importerContext.PunctualLights.size()];
        memcpy(scene->MegaBufferPunctualLights, importerContext.PunctualLights.data(), importerContext.PunctualLights.size() * sizeof(PunctualLight));
    }

    if (importerContext.KeyFrames.empty())
    {
        scene->MegaBufferKeyFramesCount = 1;
        scene->MegaBufferKeyFrames = new KeyFrame[1];
        scene->MegaBufferKeyFrames[0] = KeyFrame();
        std::cout << "No key frames in scene, added dummy" << std::endl;
    }
    else
    {
        scene->MegaBufferKeyFramesCount = importerContext.KeyFrames.size();
        scene->MegaBufferKeyFrames = new KeyFrame[importerContext.KeyFrames.size()];
        memcpy(scene->MegaBufferKeyFrames, importerContext.KeyFrames.data(), importerContext.KeyFrames.size() * sizeof(KeyFrame));
    }

    if (importerContext.AnimationClips.empty())
    {
        scene->MegaBufferAnimationClipsCount = 1;
        scene->MegaBufferAnimationClips = new AnimationClip[1];
        scene->MegaBufferAnimationClips[0] = AnimationClip();
        std::cout << "No animation clips in scene, added dummy" << std::endl;
    }
    else
    {
        scene->MegaBufferAnimationClipsCount = importerContext.AnimationClips.size();
        scene->MegaBufferAnimationClips = new AnimationClip[importerContext.AnimationClips.size()];
        memcpy(scene->MegaBufferAnimationClips, importerContext.AnimationClips.data(), importerContext.AnimationClips.size() * sizeof(AnimationClip));
    }

    std::cout << "Scene processed" << std::endl;
}
