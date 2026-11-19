#ifndef H_EXTRACT_ANIMATION_H
#define H_EXTRACT_ANIMATION_H

#pragma warning(push)
#pragma warning(disable : 4244 4305 4996 4267 4003)

#include "pxr/usd/usd/stage.h"
#include "pxr/usd/usd/primRange.h"
#include "pxr/usd/usdGeom/mesh.h"
#include "pxr/usd/usdGeom/xformCache.h"
#include "pxr/usd/usdGeom/primvarsAPI.h"
#include "pxr/usd/usdGeom/primvar.h"
#include "pxr/usd/usdGeom/pointInstancer.h"
#include "pxr/usd/usdGeom/metrics.h"
#include "pxr/usd/usdLux/lightAPI.h"
#include "pxr/usd/usdLux/distantLight.h"
#include "pxr/usd/usdShade/shader.h"
#include "pxr/usd/usdShade/tokens.h"
#include "pxr/usd/usdShade/material.h"
#include "pxr/usd/usdShade/materialBindingAPI.h"
#include "pxr/base/gf/matrix4d.h"
#include "pxr/base/gf/matrix4f.h"
#include "pxr/base/gf/rotation.h"
#include "pxr/base/gf/vec3f.h"
#include "pxr/base/tf/token.h"
#include "pxr/usd/sdf/assetPath.h"
#include <pxr/usd/usdSkel/cache.h>
#include <pxr/usd/usdSkel/root.h>
#include <pxr/usd/usdSkel/skeleton.h>
#include <pxr/usd/usdSkel/animation.h>
#include <pxr/usd/usdSkel/bindingAPI.h>

#pragma warning(pop)

#include "Importer.h"
#include "SceneLoaderUSD.h"

namespace SceneLoaderUSD
{
    inline bool ExtractAnimation(
        const pxr::UsdSkelAnimation& animation,
        const pxr::UsdSkelSkeleton& skeleton,
        const float sampleRate,
        std::vector<KeyFrame>& keyFrames,
        std::vector<AnimationClip>& animationClips)
    {
        if (!animation || !skeleton || sampleRate <= 0.0f)
            return false;

        pxr::VtArray<pxr::TfToken> skeletonJoints;

        if (!skeleton.GetJointsAttr().Get(&skeletonJoints))
            return false;

        const uint32_t boneCount = static_cast<uint32_t>(skeletonJoints.size());

        if (boneCount == 0)
            return false;

        std::unordered_map<std::string, uint32_t> skeletonJointLookup;
        skeletonJointLookup.reserve(boneCount);

        for (uint32_t i = 0; i < boneCount; ++i)
        {
            skeletonJointLookup.emplace(skeletonJoints[i].GetString(), i);
        }

        pxr::VtArray<pxr::TfToken> animationJoints;

        if (!animation.GetJointsAttr().Get(&animationJoints))
            return false;

        std::vector<uint32_t> animationToSkeleton;
        animationToSkeleton.reserve(animationJoints.size());

        for (const auto& joint : animationJoints)
        {
            auto it = skeletonJointLookup.find(joint.GetString());

            if (it == skeletonJointLookup.end())
                return false;

            animationToSkeleton.emplace_back(it->second);
        }

        pxr::UsdAttribute translationsAttr = animation.GetTranslationsAttr();
        pxr::UsdAttribute rotationsAttr = animation.GetRotationsAttr();
        pxr::UsdAttribute scalesAttr = animation.GetScalesAttr();

        std::vector<double> timeSamples;
        std::vector<double> samples;

        translationsAttr.GetTimeSamples(&timeSamples);

        rotationsAttr.GetTimeSamples(&samples);
        timeSamples.insert(timeSamples.end(), samples.begin(), samples.end());

        samples.clear();
        scalesAttr.GetTimeSamples(&samples);
        timeSamples.insert(timeSamples.end(), samples.begin(), samples.end());

        double startTime = 0.0;
        double endTime = 0.0;

        if (!timeSamples.empty())
        {
            const auto bounds = std::minmax_element(timeSamples.begin(), timeSamples.end());

            startTime = *bounds.first;
            endTime = *bounds.second;
        }

        const double durationInTimeCodes = endTime - startTime;

        const pxr::UsdStageRefPtr stage = animation.GetPrim().GetStage();

        if (!stage)
            return false;

        const double timeCodesPerSecond = stage->GetTimeCodesPerSecond();

        if (timeCodesPerSecond <= 0.0)
            return false;

        const float duration = static_cast<float>(durationInTimeCodes / timeCodesPerSecond);

        const uint32_t keyFrameCount = std::max(1u, static_cast<uint32_t>(std::ceil(duration * sampleRate)));

        if (keyFrameCount > std::numeric_limits<uint32_t>::max() / boneCount)
        {
            return false;
        }

        const size_t requiredKeyFrames = static_cast<size_t>(keyFrameCount) * boneCount;

        if (keyFrames.size() > std::numeric_limits<uint32_t>::max() - requiredKeyFrames)
        {
            return false;
        }

        // Emplace Clip
        {
            AnimationClip clip{};

            clip.Duration = duration;
            clip.MegaBufferKeyFrameOffset = keyFrames.size();

            clip.MegaBufferKeyFrameCount = keyFrameCount;
            clip.BoneCount = boneCount;

            animationClips.emplace_back(clip);
        }

        keyFrames.reserve(keyFrames.size() + requiredKeyFrames);

        for (uint32_t frame = 0; frame < keyFrameCount; ++frame)
        {
            const double sampleTime = startTime + static_cast<double>(frame) / sampleRate * timeCodesPerSecond;

            const pxr::UsdTimeCode usdTime(sampleTime);

            // Read animation components at this time.

            pxr::VtArray<pxr::GfVec3f> translations;
            pxr::VtArray<pxr::GfQuatf> rotations;
            pxr::VtArray<pxr::GfVec3h> scales;

            translationsAttr.Get(&translations, usdTime);
            rotationsAttr.Get(&rotations, usdTime);
            scalesAttr.Get(&scales, usdTime);

            std::vector<KeyFrame> frameKeyFrames(boneCount);

            // Fill with identity
            for (uint32_t bone = 0; bone < boneCount; ++bone)
            {
                KeyFrame& key = frameKeyFrames[bone];

                key.Translation[0] = 0.0f;
                key.Translation[1] = 0.0f;
                key.Translation[2] = 0.0f;

                key.Rotation[0] = 0.0f;
                key.Rotation[1] = 0.0f;
                key.Rotation[2] = 0.0f;
                key.Rotation[3] = 1.0f;

                key.Scale[0] = 1.0f;
                key.Scale[1] = 1.0f;
                key.Scale[2] = 1.0f;
            }

            // Map animation joint components into skeleton order.

            for (uint32_t j = 0; j < animationToSkeleton.size(); ++j)
            {
                const uint32_t bone = animationToSkeleton[j];

                KeyFrame& key = frameKeyFrames[bone];

                if (j < translations.size())
                {
                    const auto& t = translations[j];

                    key.Translation[0] = t[0];
                    key.Translation[1] = t[1];
                    key.Translation[2] = t[2];
                }

                if (j < rotations.size())
                {
                    const auto& r = rotations[j];

                    const pxr::GfVec3f imaginary = r.GetImaginary();

                    key.Rotation[0] = imaginary[0];
                    key.Rotation[1] = imaginary[1];
                    key.Rotation[2] = imaginary[2];
                    key.Rotation[3] = r.GetReal();
                }

                if (j < scales.size())
                {
                    const auto& s = scales[j];

                    key.Scale[0] = static_cast<float>(s[0]);
                    key.Scale[1] = static_cast<float>(s[1]);
                    key.Scale[2] = static_cast<float>(s[2]);
                }
            }

            keyFrames.insert(keyFrames.end(), frameKeyFrames.begin(), frameKeyFrames.end());
        }

        return true;
    }

    inline ImporterSkeleton ExtractSkeleton(
        const pxr::UsdPrim& prim,
        std::vector<KeyFrame>& keyFrames,
        std::vector<AnimationClip>& animationClips)
    {
        const pxr::UsdSkelSkeleton skeleton(prim);

        pxr::VtArray<pxr::TfToken> joints;
        skeleton.GetJointsAttr().Get(&joints);

        ImporterSkeleton impSkeleton{};

        if (joints.size() == 0)
            return impSkeleton;

        pxr::VtArray<pxr::GfMatrix4d> bindTransforms;
        skeleton.GetBindTransformsAttr().Get(&bindTransforms);

        pxr::VtArray<pxr::GfMatrix4d> restTransforms;
        skeleton.GetRestTransformsAttr().Get(&restTransforms);

        impSkeleton.BoneInfos.resize(joints.size());

        std::unordered_map<std::string, uint32_t> jointLookup;

        for (uint32_t i = 0; i < joints.size(); ++i)
            jointLookup[joints[i].GetString()] = i;

        for (uint32_t i = 0; i < joints.size(); ++i)
        {
            BoneInfo bone{};

            const std::string jointPath = joints[i].GetString();

            const size_t separator = jointPath.find_last_of('/');

            if (separator == std::string::npos)
            {
                bone.BoneParentIdxLocal = UINT32_MAX;
            }
            else
            {
                const std::string parentPath = jointPath.substr(0, separator);
                bone.BoneParentIdxLocal = jointLookup.at(parentPath);
            }

            assert(i < bindTransforms.size());

            const pxr::GfMatrix4f inverseBind = pxr::GfMatrix4f(bindTransforms[i].GetInverse());
            memcpy(&bone.InverseBindTransform, inverseBind.GetArray(), sizeof(float) * 16);

            impSkeleton.BoneInfos[i] = bone;
        }

        pxr::UsdSkelBindingAPI binding(skeleton.GetPrim());

        pxr::UsdPrim animPrim;
        binding.GetAnimationSource(&animPrim);
        pxr::UsdSkelAnimation animation(animPrim);

        impSkeleton.AnimationClipOffset = animationClips.size();

        ExtractAnimation(animation, skeleton, 30.0f, keyFrames, animationClips);

        impSkeleton.AnimationClipCount = animationClips.size() - impSkeleton.AnimationClipOffset;

        return impSkeleton;
    }

    inline void ExtractBoneWeights(const pxr::UsdPrim& prim, std::vector<VertexBoneWeights>& output)
    {
        pxr::UsdSkelBindingAPI binding(prim);

        pxr::VtArray<int> jointIndices;
        pxr::VtArray<float> jointWeights;

        binding.GetJointIndicesPrimvar().Get(&jointIndices);
        binding.GetJointWeightsPrimvar().Get(&jointWeights);

        if (jointIndices.size() != jointWeights.size())
            return;

        const int influencesPerVertex = binding.GetJointIndicesPrimvar().GetElementSize();
        const int maxInfluences = std::min(4, influencesPerVertex);

        const size_t vertexCount = jointIndices.size() / influencesPerVertex;

        if (influencesPerVertex <= 0 || jointIndices.size() != jointWeights.size() || jointIndices.size() % influencesPerVertex != 0)
        {
            return;
        }

        output.resize(vertexCount);

        for (size_t i = 0; i < vertexCount; ++i)
        {
            for (int j = 0; j < maxInfluences; ++j)
            {
                output[i].BoneIndices[j] = static_cast<uint32_t>(jointIndices[i * influencesPerVertex + j]);
                output[i].BoneWeights[j] = jointWeights[i * influencesPerVertex + j];
            }

            for (int j = influencesPerVertex; j < 4; ++j)
            {
                output[i].BoneIndices[j] = UINT_MAX;
                output[i].BoneWeights[j] = -1.0f;
            }
        }
    }
}

#endif